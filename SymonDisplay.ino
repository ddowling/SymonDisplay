#include "font.h"
#include <uart.h>

enum
{
    BUS0 = PA0,
    BUS1 = PA1,
    BUS2 = PA2,
    BUS3 = PA3,
    BUS4 = PA4,
    BUS5 = PA5,
    BUS6 = PA6,
    BUS7 = PA7,

    COL_SR = PB0,
    ROW_LATCH = PB1,

    LED = PC13
};

// Display is organised with one byte per column
// 64 columns but only 2 - 61 are visible. 0, 1, 62 & 63 not shown
uint8_t display[64];

void display_clear()
{
    memset(display, 0, sizeof(display));
}

void display_set_x_y(uint8_t x, uint8_t y, bool val)
{
    if (x >= sizeof(display))
        return;

    if (val)
        display[x] |= (1<<y);
    else
        display[x] &= ~(1<<y);
}

void display_toggle()
{
    // TODO
}

void update_shift_registers(uint8_t row)
{
    // Update the column shift registers
    const uint8_t num_shifts = 8;
    const uint8_t num_sr = 8;
    for (uint8_t shift = 0; shift < num_shifts; shift++)
    {
        // Set the appropriate bus bits from the display buffer
        for(uint8_t sr = 0; sr < num_sr; sr++)
        {
            uint8_t col = sr * 8 + (num_shifts - shift - 1);

            bool bit = false;
            if (col < sizeof(display))
                bit = (display[col] & (1<<row)) != 0;

            uint8_t port;
            if (sr == 0)
                port = BUS7;
            else if (sr == 1)
                port = BUS6;
            else if (sr == 2)
                port = BUS5;
            else if (sr == 3)
                port = BUS4;
            else if (sr == 4)
                port = BUS3;
            else if (sr == 5)
                port = BUS2;
            else if (sr == 6)
                port = BUS1;
            else if (sr == 7)
                port = BUS0;
            else
                continue;
            digitalWrite(port, bit);
        }
        // Move the bus bits into the shift registers
        digitalWrite(COL_SR, LOW);
        digitalWrite(COL_SR, HIGH);
    }
}

void enable_row(uint8_t row, bool on)
{
    // BUS0 nothing
    // BUS1 nothing
    // BUS2 nothinh
    // BUS3 nothing
    // BUS4 nothing
    // BUS5 = row 0
    // BUS6 = row 1
    // BUS5 | BUS6 = row2
    // BUS7 = row 3
    // BUS5 | BUS7 = row4
    // BUS6 | BUS7 = row5
    // BUS5 | BUS6 | BUS7 = row6

    bool b5 = false;
    bool b6 = false;
    bool b7 = false;
    if (on)
    {
        b5 = ((row + 1) % 2) != 0;
        b6 = (((row + 1)>>1) % 2) != 0;
        b7 = (((row + 1)>>2) % 2) != 0;
    }

    digitalWrite(BUS0, LOW);
    digitalWrite(BUS1, LOW);
    digitalWrite(BUS2, LOW);
    digitalWrite(BUS3, LOW);
    digitalWrite(BUS4, LOW);
    digitalWrite(BUS5, b5);
    digitalWrite(BUS6, b6);
    digitalWrite(BUS7, b7);

    digitalWrite(ROW_LATCH, LOW);
    digitalWrite(ROW_LATCH, HIGH);
}

uint8_t current_row = 0;

// Update event correspond to Rising edge of PWM when configured in PWM1 mode
void update_callback()
{
    enable_row(current_row, false);

    current_row = (current_row + 1) % 8;

    update_shift_registers(current_row);
    enable_row(current_row, true);
}

// Compare match event correspond to falling edge of PWM when configured in PWM1 mode
void compare_callback()
{
    enable_row(current_row, false);
}

HardwareTimer timer(TIM1);
int timer_channel = 1;

const int _width = 64;
const int _height = 8;
uint8_t font_id = 0;

// Draw a character
void display_char(int16_t x, int16_t y, char c)
{
    // Fast clip
    if((x >= _width) || // Clip right
       (y >= _height) || // Clip bottom
       ((x + 6 - 1) < 0) || // Clip left
       ((y + 8 - 1) < 0))   // Clip top
        return;

    const unsigned char *font_char = getFontChar(c, font_id);

    for (int8_t i = 0; i < 6; i++)
    {
        uint8_t v_line;
        if (i == 5)
            v_line = 0x0;
        else
            v_line = font_char[i];

        for (int8_t j = 0; j < 8; j++)
        {
            display_set_x_y(x + i, y + j, v_line&1);
            v_line >>= 1;
        }
    }
}

void display_str(int16_t x, int16_t y, const char *str)
{
    while(*str != '\0')
    {
        display_char(x, y, *str);
        str++;
        x += 6;
    }
}

void setup()
{
    Serial.begin(115200);
    Serial.println("Boot");

    // Bus pin D0 - D7 map to DIO 2 - 10
    pinMode(BUS0, OUTPUT);
    pinMode(BUS1, OUTPUT);
    pinMode(BUS2, OUTPUT);
    pinMode(BUS3, OUTPUT);
    pinMode(BUS4, OUTPUT);
    pinMode(BUS5, OUTPUT);
    pinMode(BUS6, OUTPUT);
    pinMode(BUS7, OUTPUT);

    // Column shift registers clock
    pinMode(COL_SR, OUTPUT);
    digitalWrite(COL_SR, HIGH);

    // Row select latch
    pinMode(ROW_LATCH, OUTPUT);
    digitalWrite(ROW_LATCH, HIGH);

    // Timer to drive display updates
    timer.setMode(timer_channel, TIMER_OUTPUT_COMPARE);
    timer.setOverflow(800, MICROSEC_FORMAT);
    //timer.setCaptureCompare(timer_channel, 250, MICROSEC_COMPARE_FORMAT);
    timer.setCaptureCompare(timer_channel, 10, PERCENT_COMPARE_FORMAT);
    timer.attachInterrupt(update_callback);
    timer.attachInterrupt(timer_channel, compare_callback);
    timer.resume();

    pinMode(LED, OUTPUT);

    display_clear();
    display_toggle();
}

bool cmd_mode = false;
int cmd_pos = 0;
char cmd_buffer[256];

int line_pos = 0;
char line_buffer[256];

void do_command()
{
    const char *cmd = strtok(cmd_buffer, " ");

    if (strcmp(cmd, "brightness") == 0)
    {
        const char *v_str = strtok(0, " ");

        int v = atoi(v_str);

        if (v >= 0 && v <= 100)
            timer.setCaptureCompare(timer_channel, v, PERCENT_COMPARE_FORMAT);
    }
    else if (strcmp(cmd, "reset") == 0)
    {
        // Force an STM32 reset
        NVIC_SystemReset();
    }
    else if (strcmp(cmd, "font") == 0)
    {
        const char *v_str = strtok(0, " ");

        font_id = atoi(v_str);
    }
}

void loop()
{
    if (Serial.available())
    {
        char c = Serial.read();

        bool changed = false;

        if (c == '\n' || c == '\r')
        {
            if (cmd_mode)
            {
                cmd_mode = false;
                cmd_pos = 0;
                line_pos = 0;

                do_command();
            }
            else
            {
                // Just reset to line start so the next characters will
                // start with a clear line
                line_pos = 0;
            }
        }
        else if (cmd_mode)
        {
            if (cmd_pos < sizeof(cmd_buffer))
            {
                cmd_buffer[cmd_pos++] = c;
                cmd_buffer[cmd_pos] = '\0';
            }
        }
        else if (line_pos == 0 && c == '@')
        {
            // Start of new command
            cmd_mode = true;
        }
        else if (isprint(c))
        {
            if (line_pos < sizeof(line_buffer))
            {
                line_buffer[line_pos++] = c;
                line_buffer[line_pos] = '\0';
                changed = true;
            }
        }

        if (changed)
        {
            display_clear();
            display_str(2, 0, line_buffer);
            display_toggle();
        }
    }

    bool scroll_needed = (line_pos >= 10);
}
