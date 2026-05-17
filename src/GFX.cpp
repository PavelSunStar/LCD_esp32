#include "GFX.h"

GFX::GFX(LCD_esp32& lcd) : _lcd(lcd){

}

void GFX::putPixel(int x, int y, uint16_t col){
    auto& s = _lcd._scr;

    if (!s.inited) return;
    if (x < s.x0 || y < s.y0 || x > s.x1 ||  y > s.y1) return;

    if (s.usePal || s.bpp == _8BIT){
        s.bLine8[y][x] = (uint8_t)col;
    } else {
        s.bLine16[y][x] = col;
    }
}

void GFX::hLine(int x0, int y, int x1, uint16_t col){
    auto& s = _lcd._scr;

    if (!s.inited) return;
    if (y < s.y0 || y > s.y1) return;
    if (x0 > x1) std::swap(x0, x1);
    if (x0 > s.x1 || x1 < s.x0) return; 
    
    x0 = std::max(s.x0, x0);
    x1 = std::min(s.x1, x1);

    if (s.usePal || s.bpp == _8BIT){
        memset(&s.bLine8[y][x0], col, x1 - x0 + 1);        
    } else {
        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(&s.bLine16[y][x0], col, (x1 - x0 + 1) << 1);
        } else {
            uint16_t* scr = &s.bLine16[y][x0]; 
            while (x0++ <= x1) *scr++ = col;
        }          
    }
}

void GFX::vLine(int x, int y0, int y1, uint16_t col){
    auto& s = _lcd._scr;

    if (!s.inited) return;
    if (x < s.x0 || x > s.x1) return;
    if (y0 > y1) std::swap(y0, y1);
    if (y0 > s.y1 || y1 < s.y0) return;

    int skip = s.width;        
    y0 = std::max(s.y0, y0);
    y1 = std::min(s.y1, y1);

    if (s.usePal || s.bpp == _8BIT){
        uint8_t* scr = &s.bLine8[y0][x];

        uint8_t color = (uint8_t)col;
        while (y0++ <= y1){ 
            *scr = color;
            scr += skip;
        } 
    } else {
        uint16_t* scr = &s.bLine16[y0][x];

        while (y0++ <= y1){ 
            *scr = col;
            scr += skip;
        }        
    }
}

void GFX::rect(int x0, int y0, int x1, int y1, uint16_t col){
    auto& s = _lcd._scr;

    if (!s.inited) return;

    if ((x0 == x1) && (y0 == y1)){
        putPixel(x0, y0, col);
        return;
    }

    if (x0 == x1){
        vLine(x0, y0, y0, col);
        return;
    }

    if (y1 == y0){
        hLine(x0, y0, x1, col);
        return;
    }

    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    if (x0 > s.x1 || y0 > s.y1 || x1 < s.x0 ||  y1 < s.y0){
        return;
    } else if (x0 >= s.x0 && y0 >= s.y0 && x1 <= s.x1 && y1 <= s.y1){
        int sizeX = x1 - x0 + 1; 
        int sizeY = y1 - y0 - 1;
        int width = s.width;
        int skip1 = x1 - x0; 
        int skip2 = width - x1 + x0;

        if (s.usePal || s.bpp == _8BIT){
            uint8_t* scr = &s.bLine8[y0][x0];
            uint8_t color = (uint8_t)col;

            memset(scr, color, sizeX);
            scr += width;

            while (sizeY-- > 0){
                *scr = color; scr += skip1;
                *scr = color; scr += skip2;
            }
            memset(scr, color, sizeX);
        } else {
            uint16_t* scr = &s.bLine16[y0][x0];
            uint16_t* cpy = scr;
            int lineSize = sizeX << 1;
            
            while (sizeX-- > 0) *scr++ = col;
            scr += skip2 - 1;

            while (sizeY-- > 0){
                *scr = col; scr += skip1;
                *scr = col; scr += skip2;
            }
            memcpy(scr, cpy, lineSize);
        }
    } else {
        hLine(x0, y0, x1, col); 
        hLine(x0, y1, x1, col); 
        vLine(x0, y0, y1, col);  
        vLine(x1, y0, y1, col);  
    }
}

void GFX::fillRect(int x0, int y0, int x1, int y1, uint16_t col){
    auto& s = _lcd._scr;

    if (!s.inited) return;
    
    if ((x0 == x1) && (y0 == y1)){
        putPixel(x0, y0, col);
        return;
    }

    if (x0 == x1){
        vLine(x0, y0, y1, col);
        return;
    }

    if (y1 == y0){
        hLine(x0, y0, x1, col);
        return;
    }
    
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    if (x0 > s.x1 || y0 > s.y1 || x1 < s.x0 ||  y1 < s.y0) return; 
    
    x0 = std::max(s.x0, x0);
    x1 = std::min(s.x1, x1);
    y0 = std::max(s.y0, y0);
    y1 = std::min(s.y1, y1);   
    
    int sizeX = x1 - x0 + 1;
    int sizeY = y1 - y0 + 1;     
    int width = s.width;
    
    #ifdef IS_P4
        if (!s.usePal && _lcd._ppaFill && (s.bpp == _16BIT)) { 
            ppa_fill_oper_config_t cfg = {};

            cfg.out.fill_cm = PPA_FILL_COLOR_MODE_RGB565;

            cfg.out.buffer = s.fb1;
            cfg.out.buffer_size = s.fullSize;
            cfg.out.pic_w = s.width;
            cfg.out.pic_h = s.height;

            cfg.out.block_offset_x = x0;
            cfg.out.block_offset_y = y0;
            cfg.fill_block_w = sizeX;
            cfg.fill_block_h = sizeY;

            cfg.fill_argb_color.a = 255;
            cfg.fill_argb_color.r = R16_888(col);
            cfg.fill_argb_color.g = G16_888(col);
            cfg.fill_argb_color.b = B16_888(col);  
                
            cfg.mode = PPA_TRANS_MODE_BLOCKING;
            cfg.user_data = nullptr;   

            ppa_do_fill(_lcd._ppaFill, &cfg);
            return;        
        }     
    #endif

    if (s.usePal || s.bpp == _8BIT){
        uint8_t* scr = &s.bLine8[y0][x0];
        uint8_t color = (uint8_t)col;

        while (sizeY-- > 0){
            memset(scr, color, sizeX);
            scr += width;
        }  
    } else {
        uint16_t* scr = &s.bLine16[y0][x0];
        uint16_t* cpy = scr;

        int skip = width - x1 + x0 - 1;
        int copyBytes = sizeX << 1;
            
        while (sizeX-- > 0) *scr++ = col;
        scr += skip;
        sizeY--;

        while (sizeY-- > 0){
            memcpy(scr, cpy, copyBytes);
            scr += width;
        }  
    }
}

void GFX::testRGBPanel(){
    auto& s = _lcd._scr;

    if (!s.inited) return;
    if (s.width <= 0 || s.height <= 0) return;

    const int w = s.width;
    const int h = s.height;

    const int midX = w >> 1;
    const int midY = h >> 1;

    int rectW = w / 2;
    int rectH = h / 5;

    if (rectW < 96) rectW = 96;
    if (rectH < 60) rectH = 60;

    if (rectW > w) rectW = w;
    if (rectH > h) rectH = h;

    rectH -= (rectH % 3);
    if (rectH < 3) rectH = 3;

    const int bandH = rectH / 3;

    const int cx = midX - (rectW >> 1);
    const int cy = midY - (rectH >> 1);

    const int maxX = (w > 1) ? (w - 1) : 1;
    const int maxY = (h > 1) ? (h - 1) : 1;

    // ---- фон (градиент на весь экран) ----
    for (int y = 0; y < h; y++) {
        uint8_t gy = (uint32_t)y * 255 / maxY;

        for (int x = 0; x < w; x++) {
            uint8_t rx = (uint32_t)x * 255 / maxX;

            if (s.usePal || s.bpp == 8){
                uint8_t rr = rx >> 5;
                uint8_t gg = gy >> 5;
                uint8_t bb = (255 - rx) >> 6;

                putPixel(x, y, (uint8_t)((rr << 5) | (gg << 2) | bb));
            } else {
                uint16_t rr = (uint16_t)(rx >> 3);
                uint16_t gg = (uint16_t)(gy >> 2);
                uint16_t bb = (uint16_t)((255 - rx) >> 3);

                putPixel(x, y, (rr << 11) | (gg << 5) | bb);
            }
        }
    }
/*
    // ---- белые контрольные линии ----
    hLine(0, 0, s.maxX, 0xFFFF);
    hLine(0, s.cy, s.maxX, 0xFFFF);
    hLine(0, s.maxY, s.maxX, 0xFFFF);

    vLine(0, 0, s.maxY, 0xFFFF);
    vLine(s.cx, 0, s.maxY, 0xFFFF);
    vLine(s.maxX, 0, s.maxY, 0xFFFF);

    // ---- центральный RGB-блок ----
    for (int y = 0; y < bandH; y++) {
        for (int x = 0; x < rectW; x++) {
            uint8_t v = (rectW > 1) ? ((uint32_t)x * 255 / (rectW - 1)) : 255;

            if (s.bpp == _16BIT) {
                uint16_t r = (uint16_t)(v >> 3);
                uint16_t g = (uint16_t)(v >> 2);
                uint16_t b = (uint16_t)(v >> 3);

                putPixel(cx + x, cy + y,             (r << 11));
                putPixel(cx + x, cy + y + bandH,     (g << 5));
                putPixel(cx + x, cy + y + bandH * 2, b);
            } else {
                uint8_t r = (v >> 5);
                uint8_t g = (v >> 5);
                uint8_t b = (v >> 6);

                putPixel(cx + x, cy + y,             (uint8_t)(r << 5));
                putPixel(cx + x, cy + y + bandH,     (uint8_t)(g << 2));
                putPixel(cx + x, cy + y + bandH * 2, (uint8_t)b);
            }
        }
    }

    // ---- белая рамка вокруг блока ----
    if (s.bpp == _16BIT) {
        rect(cx, cy, cx + rectW - 1, cy + rectH - 1, 0xFFFF);
    } else {
        rect(cx, cy, cx + rectW - 1, cy + rectH - 1, 0xFF);
    }
        */
}