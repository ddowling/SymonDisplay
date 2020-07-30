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

#include "font5x7.h"

const unsigned char *getFontChar(char c)
{
    return &font5x7[c*5];
    //return bmplain_font[c - ' '];
    //return first_font[c - ' '];
    //return five_five_font[c - ' '];
    //return sloth_font[c - ' '];
    //return haiku_font[c - ' '];
    //return renew_font[c - ' '];
}
