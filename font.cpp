#include "font.h"

// Works with y offset -1
#define font first_font
#include "fonts/font.h"
#undef font

// Works
#define font five_five_font
#include "fonts/5x5_font.h"
#undef font

// Works
#define font bmplain_font
#include "fonts/BMplain_font.h"
#undef font

// FIXME
#define font sloth_font
#include "fonts/sloth_font.h"
#undef font

// FIXME
#define font haiku_font
#include "fonts/haiku_font.h"
#undef font

// FIXME Could reduce spacing on some characters
#define font renew_font
#include "fonts/renew_font.h"
#undef font

// Works
#include "font5x7.h"

const unsigned char *getFontChar(char c, uint8_t font_id)
{
    switch(font_id)
    {
    case 0:
    default:
        return &font5x7[c*5];
    case 1:
        return bmplain_font[c - ' '];
    case 2:
        return first_font[c - ' '];
    case 3:
        return five_five_font[c - ' '];
    case 4:
        return sloth_font[c - ' '];
    case 5:
        return haiku_font[c - ' '];
    case 6:
        return renew_font[c - ' '];
    }
}
