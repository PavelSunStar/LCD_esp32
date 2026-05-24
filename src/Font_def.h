#pragma once

#include "LCD_esp32.h"
#include "fonts\default.h"

class Font_def{
    public:
        Font_def(LCD_esp32 &lcd);
        ~Font_def();

        uint8_t*    PAL8()  { return _pal8; }
        uint16_t*   PAL16() { return _pal16; }
        uint8_t     Pal8Col(uint8_t index) { return _pal8[index]; }
        uint16_t    Pal16Col(uint8_t index) { return _pal16[index]; }

        bool initPal();
        void shiftPal(void* pal, uint8_t first, uint8_t last, uint8_t step, bool cc);
        void makeGold(uint16_t* pal);
        void makeFire(uint16_t* pal);

        void putChar(int x, int y, char ch, uint16_t col);
        void putAffineChar(int dstX, int dstY, char ch, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY);
        void putString(int x, int y, const char* str, uint16_t col);
        void putAffineString(int dstX, int dstY, const char* str, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY);        
        void putAffineCharPal(int dstX, int dstY, char ch, float ang, uint16_t zoomX, uint16_t zoomY);

        void putInt(int x, int y, int value, uint16_t col);
        void putFloat(int x, int y, float value, uint16_t col, int precision);
        

    private:
        const uint8_t* _buf;

        uint8_t*    _pal8 = nullptr;
        uint16_t*   _pal16 = nullptr;
        bool        _palInited = false;
        uint8_t     _palOffset;

        void freePal();

        LCD_esp32 &_lcd;        
};