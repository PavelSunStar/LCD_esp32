#pragma once

#include "LCD_esp32.h"

class GFX{
    public:
        GFX(LCD_esp32& lcd);

        uint16_t getPixel(int x, int y);
        void putPixel(int x, int y, uint16_t col);
        inline void putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b);
        void PutPixelAlpha(int x, int y, uint16_t col, uint8_t alpha);
        void hLine(int x0, int y, int x1, uint16_t col);
        inline void hLineLen(int x, int y, int len, uint16_t col);
        void vLine(int x, int y0, int y1, uint16_t col);
        inline void vLineLen(int x, int y, int h, uint16_t col);
        void rect(int x0, int y0, int x1, int y1, uint16_t col);
        void fillRect(int x0, int y0, int x1, int y1, uint16_t col);
        void line(int x0, int y0, int x1, int y1, uint16_t col);
        void lineAA(float x0, float y0, float x1, float y1, uint16_t col);
        void lineAAThick(float x0, float y0, float x1, float y1, int thick, uint16_t col);
        inline void triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col);
        void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col);
        void circle(int xc, int yc, int r, uint16_t col);
        void fillCircle(int xc, int yc, int r, uint16_t col);
        void star(int x, int y, int radius, int sides, int rotation, uint16_t col);
        void fillStar(int x, int y, int radius, int sides, int rotation, uint16_t col);

        void blur();
        void testRGBPanel();

    private:
        LCD_esp32 &_lcd;
};    