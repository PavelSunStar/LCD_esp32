#pragma once

#include "LCD_esp32.h"

class GFX{
    public:
        GFX(LCD_esp32& lcd);

        void putPixel(int x, int y, uint16_t col);
        void hLine(int x0, int y, int x1, uint16_t col);
        void vLine(int x, int y0, int y1, uint16_t col);
        void rect(int x0, int y0, int x1, int y1, uint16_t col);
        void fillRect(int x0, int y0, int x1, int y1, uint16_t col);

        void testRGBPanel();

    private:
        LCD_esp32 &_lcd;
};    