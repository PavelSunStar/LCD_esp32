#include "Font_def.h"
#include "Matrix.h"

Font_def::Font_def(LCD_esp32 &lcd) : _lcd(lcd){
    _buf = FONT_8x8_Def;
    _buf += 2;
}

Font_def::~Font_def(){
    freePal();
}

void Font_def::freePal(){
    if (_pal16){
        free(_pal16);
        _pal16 = nullptr;
    }

    if (_pal8){
        free(_pal8);
        _pal8 = nullptr;
    }

    _palInited = false;
}

bool Font_def::initPal(){
    auto& s = _lcd._scr;
    if (!s.inited) return false;

    freePal();

    if (s.bpp == 16){
        _pal16 = (uint16_t*)malloc(256 * sizeof(uint16_t));
        if (!_pal16) return false;

        // 🔥 HSV -> RGB565 (радужная палитра)
        for (int i = 0; i < 256; i++){
            uint8_t r, g, b;

            uint8_t region = i / 43;
            uint8_t remainder = (i - region * 43) * 6;

            uint8_t p = 0;
            uint8_t q = (255 * (255 - remainder)) >> 8;
            uint8_t t = (255 * remainder) >> 8;

            switch (region){
                case 0: r = 255; g = t;   b = 0;   break;
                case 1: r = q;   g = 255; b = 0;   break;
                case 2: r = 0;   g = 255; b = t;   break;
                case 3: r = 0;   g = q;   b = 255; break;
                case 4: r = t;   g = 0;   b = 255; break;
                default:r = 255; g = 0;   b = q;   break;
            }

            _pal16[i] =
                ((r & 0xF8) << 8) |
                ((g & 0xFC) << 3) |
                ( b >> 3);
        }

    } else {
        _pal8 = (uint8_t*)malloc(256);
        if (!_pal8) return false;

        // просто identity
        for (int i = 0; i < 256; i++){
            _pal8[i] = i;
        }
    }

    return (_palInited = true);
}

void Font_def::shiftPal(void* pal, uint8_t first, uint8_t last, uint8_t step, bool cc){
    auto& s = _lcd._scr;
    if (!_palInited || !pal || step == 0) return;

    if (first > last) std::swap(first, last);

    uint32_t size = last - first + 1;
    step %= size;
    if (step == 0) return;

    if (s.bpp == 16){
        uint16_t* p = (uint16_t*)pal;
        uint16_t tmp[256];

        uint32_t shiftBytes = step << 1;           // step * 2
        uint32_t keepBytes  = (size - step) << 1;  // (size-step) * 2

        if (cc){
            // ←←←
            memcpy(tmp, p + first, shiftBytes);
            memmove(p + first, p + first + step, keepBytes);
            memcpy(p + first + (size - step), tmp, shiftBytes);
        } else {
            // →→→
            memcpy(tmp, p + last - step + 1, shiftBytes);
            memmove(p + first + step, p + first, keepBytes);
            memcpy(p + first, tmp, shiftBytes);
        }

    } else {
        uint8_t* p = (uint8_t*)pal;
        uint8_t tmp[256];

        if (cc){
            memcpy(tmp, p + first, step);
            memmove(p + first, p + first + step, size - step);
            memcpy(p + first + (size - step), tmp, step);
        } else {
            memcpy(tmp, p + last - step + 1, step);
            memmove(p + first + step, p + first, size - step);
            memcpy(p + first, tmp, step);
        }
    }
}

void Font_def::makeGold(uint16_t* pal){
    for (int i = 0; i < 256; i++){
        uint8_t v = i;
        uint8_t r = v;
        uint8_t g = v * 0.7;
        uint8_t b = v * 0.1;

        pal[i] =
            ((r & 0xF8) << 8) |
            ((g & 0xFC) << 3) |
            ( b >> 3);
    }
}

void Font_def::makeFire(uint16_t* pal){
    for (int i = 0; i < 256; i++){
        uint8_t r = i;
        uint8_t g = i > 128 ? (i - 128) * 2 : 0;
        uint8_t b = 0;

        pal[i] =
            ((r & 0xF8) << 8) |
            ((g & 0xFC) << 3) |
            ( b >> 3);
    }
}

void Font_def::putChar(int x, int y, char ch, uint16_t col){
    auto& s = _lcd._scr;
    if (!s.inited || !_buf) return;

    if (x > s.x1 || y > s.y1) return;
    int xx = x + 7;
    int yy = y + 7;
    if (xx < s.x0 || yy < s.y0) return;

    const uint8_t* c = _buf + (((uint8_t)ch) << 3);

    if (x >= s.x0 && y >= s.y0 && xx <= s.x1 && yy <= s.y1){
        int skip = s.width - 8;

        if (s.usePal || s.bpp == _8BIT){
            uint8_t color = (uint8_t)col;
            uint8_t* scr = &s.bLine8[y][x];

            while (y++ <= yy){
                uint8_t line = *c++;

                for (int px = 0; px < 8; px++){
                    if (line & 0x80) *scr = color;

                    line <<= 1;
                    scr++;
                }

                scr += skip;
            }
        } else {
            uint16_t* scr = &s.bLine16[y][x];

            while (y++ <= yy){
                uint8_t line = *c++;

                for (int px = 0; px < 8; px++){
                    if (line & 0x80) *scr = col;

                    line <<= 1;
                    scr++;
                }
                
                scr += skip;
            }
        }

        if (s.bpp == _16BIT){

        } else {

        }
    } else {
        int sxl = (x < s.x0     ? s.x0 - x  : 0);
        int sxr = (xx > s.x1    ? xx - s.x1 : 0);
        int syu = (y < s.y0     ? s.y0 - y  : 0);
        int syd = (yy > s.y1    ? yy - s.y1 : 0);

        int copyX = 8 - sxl - sxr;
        int copyY = 8 - syu - syd;
        if (copyX <= 0 || copyY <= 0) return;

        if (s.usePal || s.bpp == _8BIT){
            uint8_t color = (uint8_t)col;
            uint8_t* scr = &s.bLine8[y + syu][x + sxl];
            int skipScr = s.width - copyX;

            c += syu;

            while (copyY-- > 0){
                uint8_t line = *c++;
                line <<= sxl;

                for (int px = 0; px < copyX; px++){
                    if (line & 0x80) *scr = color;
                    line <<= 1;
                    scr++;
                }

                scr += skipScr;
            }
        } else {
            uint16_t* scr = &s.bLine16[y + syu][x + sxl];
            int skipScr = s.width - copyX;

            c += syu;

            while (copyY-- > 0){
                uint8_t line = *c++;
                line <<= sxl;

                for (int px = 0; px < copyX; px++){
                    if (line & 0x80) *scr = col;
                    line <<= 1;
                    scr++;
                }

                scr += skipScr;
            }
        }
    }
}

void Font_def::putAffineChar(int dstX, int dstY, char ch, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY){
    auto& s = _lcd._scr;
    if (!s.inited || !_buf) return;

    Affine2D m = Matrix::make(  (float)dstX, (float)dstY,
                                4.0f, 4.0f,
                                angle(ang),
                                percentTo(zoomX), percentTo(zoomY)
    );
    
    RectBounds rb = Matrix::bounds(m, 8.0f, 8.0f);
    int x0 = (int)floorf(rb.sx);    // округляет число вниз до ближайшего целого
    int y0 = (int)floorf(rb.sy);
    int x1 = (int)ceilf(rb.ex);     // округляет число вверх до ближайшего целого
    int y1 = (int)ceilf(rb.ey);

    x0 = std::max(x0, s.x0);
    y0 = std::max(y0, s.y0);
    x1 = std::min(x1, s.x1);
    y1 = std::min(y1, s.y1);
    if (x0 >= x1 || y0 >= y1) return;

    Affine2DInv inv;
    if (!Matrix::invert(inv, m)) return;

    int u, v;
    int sx, sy;
    int row_u = inv.a * x0 + inv.b * y0 + inv.c;
    int row_v = inv.d * x0 + inv.e * y0 + inv.f;
    int width = s.width;

    const uint8_t* c = _buf + (((uint8_t)ch) << 3);
    if (s.usePal || s.bpp == _8BIT){
        uint8_t color = (uint8_t)col;
        uint8_t* dstBase = &s.bLine8[y0][x0];     
        
        while (y0++ < y1){
            uint8_t* dst = dstBase;
            u = row_u;
            v = row_v;

            for (int x = x0; x < x1; x++){
                sx = (u >> FP_SHIFT);
                sy = (v >> FP_SHIFT);
                if ((unsigned)sx < 8u && (unsigned)sy < 8u){
                    uint8_t line = c[sy];
                    if (line & (0x80 >> sx)) *dst = color;
                } 
                
                ++dst;
                u += inv.a;
                v += inv.d;                    
            }

            dstBase += width;
            row_u   += inv.b;
            row_v   += inv.e;   
        }
    } else {
        uint16_t* dstBase = &s.bLine16[y0][x0];

        while (y0++ < y1){
            uint16_t* dst = dstBase;
            u = row_u;
            v = row_v;

            for (int x = x0; x < x1; x++){
                sx = (u >> FP_SHIFT);
                sy = (v >> FP_SHIFT);
                if ((unsigned)sx < 8u && (unsigned)sy < 8u){
                    uint8_t line = c[sy];
                    if (line & (0x80 >> sx)) *dst = col;
                } 
                
                ++dst;
                u += inv.a;
                v += inv.d;                    
            }

            dstBase += width;
            row_u   += inv.b;
            row_v   += inv.e;   
        }
    }
}

void Font_def::putAffineCharPal(int dstX, int dstY, char ch, float ang, uint16_t zoomX, uint16_t zoomY){
    auto& s = _lcd._scr;
    if (!s.inited || !_buf || !_palInited) return;

    Affine2D m = Matrix::make(
        (float)dstX, (float)dstY,
        3.5f, 3.5f,
        angle(ang),
        percentTo(zoomX), percentTo(zoomY)
    );

    RectBounds rb = Matrix::bounds(m, 8.0f, 8.0f);

    int x0 = (int)floorf(rb.sx);
    int y0 = (int)floorf(rb.sy);
    int x1 = (int)ceilf(rb.ex);
    int y1 = (int)ceilf(rb.ey);

    x0 = std::max(x0, s.x0);
    y0 = std::max(y0, s.y0);
    x1 = std::min(x1, s.x1);
    y1 = std::min(y1, s.y1);
    if (x0 >= x1 || y0 >= y1) return;

    Affine2DInv inv;
    if (!Matrix::invert(inv, m)) return;

    int u, v;
    int sx, sy;

    int32_t row_u = inv.a * x0 + inv.b * y0 + inv.c;
    int32_t row_v = inv.d * x0 + inv.e * y0 + inv.f;

    int width = s.width;

    const uint8_t* c = _buf + (((uint8_t)ch) << 3);
    if (s.usePal || s.bpp == _8BIT){
        uint8_t* dstBase = &s.bLine8[y0][x0];

        while (y0++ < y1){
            uint8_t* dst = dstBase;
            u = row_u;
            v = row_v;

            for (int x = x0; x < x1; x++){
                sx = (u >> FP_SHIFT);
                sy = (v >> FP_SHIFT);

                if ((unsigned)sx < 8u && (unsigned)sy < 8u){
                    uint8_t line = c[sy];

                    if (line & (0x80 >> sx)){
                        uint8_t idx = ((u >> 10) + (v >> 10)) & 0xFF;
                        *dst = idx; // индекс
                    }
                }

                ++dst;
                u += inv.a;
                v += inv.d;
            }

            dstBase += width;
            row_u   += inv.b;
            row_v   += inv.e;
        }
    } else {
        uint16_t* dstBase = &s.bLine16[y0][x0];

        while (y0++ < y1){
            uint16_t* dst = dstBase;
            u = row_u;
            v = row_v;

            for (int x = x0; x < x1; x++){
                sx = (u >> FP_SHIFT);
                sy = (v >> FP_SHIFT);

                if ((unsigned)sx < 8u && (unsigned)sy < 8u){
                    uint8_t line = c[sy];

                    if (line & (0x80 >> sx)){
                        // 🔥 цвет из u/v
                        //uint8_t idx = ((u >> 10) + (v >> 10)) & 0xFF;
                        uint8_t idx = (((u >> 10) + (v >> 10) + 0) & 0xFF);
                        //uint8_t idx = (sy << 5);
                        *dst = _pal16[idx];
                    }
                }

                ++dst;
                u += inv.a;
                v += inv.d;
            }

            dstBase += width;
            row_u   += inv.b;
            row_v   += inv.e;
        }
    }

    _palOffset++;
}

void Font_def::putString(int x, int y, const char* str, uint16_t col){
    auto& s = _lcd._scr;
    if (!s.inited || !_buf || !str) return;

    int startX = x;

    while (*str){
        char ch = *str++;

        if (ch == '\n'){
            x = startX;
            y += 8;
            continue;
        }

        if (ch == '\r'){
            continue;
        }

        putChar(x, y, ch, col);
        x += 8;
    }
}

void Font_def::putAffineString(int dstX, int dstY, const char* str, uint16_t col, float ang, uint16_t zoomX, uint16_t zoomY){
    auto& s = _lcd._scr;
    if (!s.inited || !_buf || !str) return;

    float rad = angle(ang);
    float zx  = percentTo(zoomX);
    float zy  = percentTo(zoomY);

    float cs = cosf(rad);
    float sn = sinf(rad);

    float advanceX = 8.0f * zx * cs;
    float advanceY = 8.0f * zx * sn;

    float lineX = -8.0f * zy * sn;
    float lineY =  8.0f * zy * cs;

    float baseX = (float)dstX;
    float baseY = (float)dstY;
    float penX  = baseX;
    float penY  = baseY;

    while (*str){
        char ch = *str++;

        if (ch == '\n'){
            baseX += lineX;
            baseY += lineY;
            penX = baseX;
            penY = baseY;
            continue;
        }

        if (ch == '\r'){
            continue;
        }

        putAffineChar((int)penX, (int)penY, ch, col, ang, zoomX, zoomY);

        penX += advanceX;
        penY += advanceY;
    }
}

void Font_def::putInt(int x, int y, int value, uint16_t col){
    char buf[12];
    int i = 0;

    if (value == 0){
        putChar(x, y, '0', col);
        return;
    }

    bool neg = value < 0;
    if (neg) value = -value;

    // собираем цифры наоборот
    while (value > 0){
        buf[i++] = '0' + (value % 10);
        value /= 10;
    }

    if (neg) buf[i++] = '-';

    // выводим в правильном порядке
    while (i--){
        putChar(x, y, buf[i], col);
        x += 8;
    }
}

void Font_def::putFloat(int x, int y, float value, uint16_t col, int precision){
    // знак
    if (value < 0){
        putChar(x, y, '-', col);
        x += 8;
        value = -value;
    }

    // целая часть
    int intPart = (int)value;
    putInt(x, y, intPart, col);

    // сдвиг по X после целой части
    int temp = intPart;
    int digits = (temp == 0) ? 1 : 0;
    while (temp){
        temp /= 10;
        digits++;
    }
    x += digits * 8;

    // точка
    putChar(x, y, '.', col);
    x += 8;

    // дробная часть
    float frac = value - (float)intPart;

    for (int i = 0; i < precision; i++){
        frac *= 10.0f;
        int digit = (int)frac;
        putChar(x, y, '0' + digit, col);
        x += 8;
        frac -= digit;
    }
}