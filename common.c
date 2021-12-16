#include <stdio.h>

#include "tft_gfx.h"
#include "tft_master.h"

#include "common.h"

void tft_log(const char *format, ...) {
    char buffer[100];
    
    va_list args;
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    
    if (cursor_y >= tft_height()) {
        tft_setCursor(0, 0);
    }
    
    tft_setTextColor2(ILI9340_YELLOW, ILI9340_BLACK);
    tft_setTextSize(2);
    
    tft_writeString(buffer);
}
