#include "Sprite.h"

Sprite::Sprite(LCD_esp32& lcd) : _lcd(lcd){
}

Sprite::~Sprite() {
    destroy();
}

void Sprite::destroy(){
    if (_img)       { delete[] _img; _img = nullptr; }
    
    _buf = nullptr;
    if (_line8)     { delete[] _line8; _line8 = nullptr; }
    if (_line16)    { delete[] _line16; _line16 = nullptr; }   
}

void Sprite::resetParam(uint8_t num){
    if (num == 0) return;

    destroy();
    _images = num;

    if (_img){
        delete[] _img;
        _img = nullptr;
    }

    _img = new Image[_images];
}

bool Sprite::setBufferAddr(){
    if (!_buf || !_img || _images == 0) {
        Serial.println("setBufferAddr: invalid sprite buffer");
        return false;
    }

    if (_line8) {
        delete[] _line8;
        _line8 = nullptr;
    }

    if (_line16) {
        delete[] _line16;
        _line16 = nullptr;
    }

    int totalLines = 0;
    for (int i = 0; i < _images; i++) totalLines += _img[i].height;

    int line = 0;
    if (_bpp == _16BIT){
        uint16_t* base = (uint16_t*)_buf;
        _line16 = new uint16_t*[totalLines];

        for (int i = 0; i < _images; i++){
            Image &im = _img[i];
            for (int y = 0; y < im.height; y++){
                _line16[line++] = base;
                base += im.width;
            }
        }
    } else {
        uint8_t* base = _buf;
        _line8 = new uint8_t*[totalLines];

        for (int i = 0; i < _images; i++){
            Image &im = _img[i];
            for (int y = 0; y < im.height; y++){
                _line8[line++] = base;
                base += im.width;
            }
        }
    }

    return true;
}

bool Sprite::loadImages(const uint8_t* data){
    if (!data) return false;

    uint8_t imgBPP = *data++;
    if (imgBPP != _8BIT && imgBPP != _16BIT) return false;
    resetParam(*data++);

    data += _images << 2;
    const uint8_t* ptr = data;    

    int fullSize = 0;
    uint32_t offsetLine = 0;
    _bpp = (_lcd._scr.usePal || _lcd._scr.bpp == _8BIT) ? _8BIT : _16BIT;
    _shift = (_bpp == _8BIT ? 0 : 1);
    int imgShift = (imgBPP == _16BIT ? 1 : 0); 

    for (int i = 0; i < _images; i++){
        _img[i].width       = (*data++) | (*data++ << 8);
        _img[i].height      = (*data++) | (*data++ << 8);
        _img[i].maxX        = _img[i].width - 1;
        _img[i].maxY        = _img[i].height - 1;
        _img[i].cx          = _img[i].width >> 1;
        _img[i].cy          = _img[i].height >> 1;
        _img[i].lineSize    = _img[i].width << _shift;
        _img[i].size        = _img[i].width * _img[i].height;
        _img[i].fullSize    = _img[i].lineSize * _img[i].height;
        _img[i].offset      = fullSize;
        _img[i].offsetLine  = offsetLine;

        offsetLine  += _img[i].height;
        fullSize    += _img[i].fullSize;
        data        += (_img[i].width << imgShift) * _img[i].height;
    }
    _buf = (uint8_t*) _lcd.allocateMemory(fullSize, true);
    if (!_buf) return false;

    if (!setBufferAddr()) return false;

    for (int i = 0; i < _images; i++){
        ptr += 4;
        int size = _img[i].size;

        if (imgBPP == _bpp){
            memcpy(_buf + _img[i].offset, ptr, _img[i].fullSize);   // Convert not needed
            ptr += _img[i].fullSize; 
        } else if (imgBPP == _8BIT && _bpp == _16BIT){                      // Convert 8 -> 16
            uint16_t* dest = (uint16_t*)(_buf + _img[i].offset);

            while (size-- > 0) *dest++ = rgb332to565(*ptr++);
        } else if (imgBPP == _16BIT && _bpp == _8BIT){
            uint16_t* sour = (uint16_t*)ptr;
            uint8_t* dest = _buf + _img[i].offset;
            
            while (size-- > 0) *dest++ = rgb565to332(*sour++);                        
            ptr += _img[i].fullSize;
        }
    }

    return (_created = true);
}

void Sprite::putImage(int x, int y, uint8_t num){
    if (!_created || num >= _images) return;

    auto& s = _lcd._scr;
    Image& im = _img[num];

    if ((unsigned)x > (unsigned)s.x1 || (unsigned)y > (unsigned)s.y1) return;

    int xx = x + im.maxX;
    int yy = y + im.maxY;
    if (xx < s.x0 || yy < s.y0) return;

    // === FAST PATH: sprite fully inside viewport ===
    if (x >= s.x0 && y >= s.y0 && xx <= s.x1 && yy <= s.y1){
        int lines = im.height;

        if (_bpp == _16BIT){
            uint8_t* img = _buf + im.offset;
            uint8_t* scr = (uint8_t*)s.bLine16[y] + (x << 1);
            const int copyBytes = im.width << 1;
            const int dstSkip = s.lineSize;

            while (lines--){
                memcpy(scr, img, copyBytes);
                img += copyBytes;
                scr += dstSkip;
            }
        } else {
            uint8_t* img = _buf + im.offset;
            uint8_t* scr = (uint8_t*)s.bLine8[y] + x;
            const int copyBytes = im.width;
            const int dstSkip = s.lineSize;

            while (lines--){
                memcpy(scr, img, copyBytes);
                img += copyBytes;
                scr += dstSkip;
            }
        }
        return;
    }

    // === CLIPPED PATH ===
    int sxl = (x  < s.x0 ? (s.x0 - x)  : 0);
    int sxr = (xx > s.x1 ? (xx - s.x1) : 0);
    int syu = (y  < s.y0 ? (s.y0 - y)  : 0);
    int syd = (yy > s.y1 ? (yy - s.y1) : 0);

    int copyX = (im.width - sxl - sxr) << _shift;
    int copyY =  im.height - syu - syd;
    if (copyX <= 0 || copyY <= 0) return;

    uint8_t* img = (_bpp == _16BIT)
        ? (uint8_t*)_line16[im.offsetLine + syu] + (sxl << 1)
        : (uint8_t*)_line8 [im.offsetLine + syu] + sxl;

    uint8_t* scr = (_bpp == _16BIT)
        ? (uint8_t*)s.bLine16[y + syu] + ((x + sxl) << 1)
        : (uint8_t*)s.bLine8 [y + syu] + (x + sxl);

    const int imgStep = im.lineSize;
    const int scrStep = s.lineSize;

    while (copyY--){
        memcpy(scr, img, copyX);
        img += imgStep;
        scr += scrStep;
    }
}

void Sprite::putAffineSprite(int dstX, int dstY, float ang, int zoomX, int zoomY, uint16_t maskColor, uint8_t num){
    auto& s = _lcd._scr;
    if (!s.inited || !_created || num >= _images) return;
 
    Image& im = _img[num];
    const int sourW = im.width;
    const int sourH = im.height;    

    mat = Matrix::make((float)dstX, (float)dstY, 
                    (sourW - 1) * 0.5f, (sourH - 1) * 0.5f, 
                    angle(ang), 
                    percentTo(zoomX), percentTo(zoomY)
    );
    
    rectMat = Matrix::bounds(mat, (float)sourW, (float)sourH);
    int x0 = (int)floorf(rectMat.sx); // округляет число вниз до ближайшего целого
    int y0 = (int)floorf(rectMat.sy);
    int x1 = (int)ceilf(rectMat.ex);  // округляет число вверх до ближайшего целого.
    int y1 = (int)ceilf(rectMat.ey);  

    x0 = std::max(x0, s.x0);
    y0 = std::max(y0, s.y0);
    x1 = std::min(x1, s.x1 + 1);
    y1 = std::min(y1, s.y1 + 1);
    if (x0 >= x1 || y0 >= y1) return;

    if (!Matrix::invert(invMat, mat)) return; 
    int row_u = invMat.a * x0 + invMat.b * y0 + invMat.c;
    int row_v = invMat.d * x0 + invMat.e * y0 + invMat.f;    

    int u, v, sx, sy;
    if (s.usePal || s.bpp == _8BIT){
        uint8_t** srcLines = _line8 + im.offsetLine;
        uint8_t* dstBase   = &s.bLine8[y0][x0];
        const uint8_t mask = (uint8_t)maskColor;

        while (y0++ < y1){
            uint8_t* dst = dstBase;
            int32_t u = row_u;
            int32_t v = row_v;

            for (int x = x0; x < x1; x++){
                int sx = u >> FP_SHIFT;
                int sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)sourW &&
                    (unsigned)sy < (unsigned)sourH)
                {
                    uint8_t* srcRow = srcLines[sy];
                    uint8_t col = srcRow[sx];
                    if (col != mask) *dst = col;
                }

                ++dst;
                u += invMat.a;
                v += invMat.d;
            }

            dstBase += s.width;
            row_u   += invMat.b;
            row_v   += invMat.e;
        }  
    } else {
        uint16_t** sour = _line16 + im.offsetLine;
        uint16_t* dest = &s.bLine16[y0][x0];

        while (y0++ < y1){
            uint16_t* dst = dest;
            u = row_u;
            v = row_v;

            for (int x = x0; x < x1; x++){
                sx = u >> FP_SHIFT;
                sy = v >> FP_SHIFT;

                if ((unsigned)sx < (unsigned)sourW && (unsigned)sy < (unsigned)sourH){
                    uint16_t col = sour[sy][sx];
                    if (col != maskColor) *dst = col;
                }

                dst++;
                u += invMat.a;
                v += invMat.d;                
            }

            dest += s.width;
            row_u   += invMat.b;
            row_v   += invMat.e;            
        }
    }
}    