#pragma once

#include "LCD_esp32.h"
#include "Matrix.h"

class Sprite{
    public:
        Sprite(LCD_esp32& lcd);
        ~Sprite();

        bool loadImages(const uint8_t* data);
        void putImage(int x, int y, uint8_t num = 0);
        void putAffineSprite(int dstX, int dstY, float ang = 0, int zoomX = 100, int zoomY = 100, uint16_t maskColor = 0, uint8_t num = 0);

    private:
        Image* _img;
        bool _created = false;

        uint8_t*    _buf    = nullptr;
        uint8_t**   _line8  = nullptr;
        uint16_t**  _line16 = nullptr;

        uint8_t _bpp;
        int     _shift;
        uint8_t _images = 0;

        LCD_esp32 &_lcd;

    protected:
        void destroy();
        void resetParam(uint8_t num);
        bool setBufferAddr();

        Affine2D mat;
        RectBounds rectMat;  
        Affine2DInv invMat;    
};    