#include "GFX.h"

GFX::GFX(LCD_esp32& lcd) : _lcd(lcd){

}

uint16_t GFX::getPixel(int x, int y){
    auto& s = _lcd._scr;

    if (!s.inited) return 0;
    if (x < s.x0 || y < s.y0 || x > s.x1 ||  y > s.y1) return 0;

    if (s.usePal || s.bpp == _8BIT){
        return (uint16_t)s.bLine8[y][x];
    } else {
        return s.bLine16[y][x];
    }   
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

inline void GFX::putPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b){
    putPixel(x, y, rgb888to565(r, g, b));
}

void GFX::PutPixelAlpha(int x, int y, uint16_t col, uint8_t alpha){
    auto& s = _lcd._scr;
    if (!s.inited || s.usePal || s.bpp == _8BIT) return;
    if (x < s.x0 || y < s.y0 || x > s.x1 || y > s.y1) return;

    uint16_t bg  = getPixel(x, y);
    uint16_t out = calcAlpha(col, bg, alpha);

    s.bLine16[y][x] = out;
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

inline void GFX::hLineLen(int x, int y, int len, uint16_t col){
    if (len <= 0) return;
    hLine(x, y, x + len - 1, col);
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

inline void GFX::vLineLen(int x, int y, int len, uint16_t col){
    if (len <= 0) return;
    vLine(x, y, y + len - 1, col);
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
        if ((!s.usePal && _lcd._ppaFill) || (s.bpp == _16BIT)) { 
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

void GFX::line(int x0, int y0, int x1, int y1, uint16_t col){
    auto& s = _lcd._scr;
    if (!s.inited) return;

    bool    steep = abs(y1 - y0) > abs(x1 - x0); 
    int     xstart = s.x0;
    int     ystart = s.y0;
    int     xend   = s.x1;
    int     yend   = s.y1;

    if (steep){
        std::swap(xstart, ystart);
        std::swap(xend, yend);
        std::swap(x0, y0);
        std::swap(x1, y1);
    } if (x0 > x1){
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    if (x0 > xend || x1 < xstart) return;
    xend = std::min(x1, xend);
    
    int     dy = abs(y1 - y0);
    int     ystep = (y1 > y0) ? 1 : -1;
    int     dx = x1 - x0;
    int     err = dx >> 1;    

    while (x0 < xstart || y0 < ystart || y0 > yend){
        err -= dy;
        if (err < 0){
            err += dx;
            y0 += ystep;
        }
        
        if (++x0 > xend) return;
    } 
    int     xs = x0;
    int     dlen = 0;
    if (ystep < 0) std::swap(ystart, yend);
    yend += ystep;    
    
    if (steep){
        do{
            ++dlen;
            if ((err -= dy) < 0){
                vLineLen(y0, xs, dlen, col);
                err += dx;
                xs = x0 + 1; dlen = 0; y0 += ystep;
                if (y0 == yend) break;
            }
        } while (++x0 <= xend);

      if (dlen) vLineLen(y0, xs, dlen, col);
    } else {
        do{
            ++dlen;
            if ((err -= dy) < 0)
            {
            hLineLen(xs, y0, dlen, col);
            err += dx;
            xs = x0 + 1; dlen = 0; y0 += ystep;
            if (y0 == yend) break;
            }
      } while (++x0 <= xend);

      if (dlen) hLineLen(xs, y0, dlen, col);
    }    
}

inline void GFX::triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t col){
    line(x0, y0, x1, y1, col);
    line(x1, y1, x2, y2, col);
    line(x2, y2, x0, y0, col);
}

void GFX::fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3, uint16_t col){
    // Сортируем вершины по y (y1 <= y2 <= y3)
    if (y1 > y2){ std::swap(y1, y2); std::swap(x1, x2); }
    if (y1 > y3){ std::swap(y1, y3); std::swap(x1, x3); }
    if (y2 > y3){ std::swap(y2, y3); std::swap(x2, x3); }

    auto drawScanline = [&](int y, int xStart, int xEnd){
        hLine(xStart, y, xEnd, col);
    };

    auto edgeInterp = [](int y1, int x1, int y2, int x2, int y) -> int {
        if (y2 == y1) return x1; // избежать деления на 0
        return x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    };

    // Верхняя часть треугольника
    for (int y = y1; y <= y2; y++){
        int xa = edgeInterp(y1, x1, y3, x3, y);
        int xb = edgeInterp(y1, x1, y2, x2, y);
        if (xa > xb) std::swap(xa, xb);
        drawScanline(y, xa, xb);
    }

    // Нижняя часть треугольника
    for (int y = y2 + 1; y <= y3; y++){
        int xa = edgeInterp(y1, x1, y3, x3, y);
        int xb = edgeInterp(y2, x2, y3, x3, y);
        if (xa > xb) std::swap(xa, xb);
        drawScanline(y, xa, xb);
    }
}

void GFX::circle(int xc, int yc, int r, uint16_t col){
    if (!_lcd._scr.inited || r < 0) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    int32_t f = 1 - r;
    int32_t ddF_y = - (r << 1);
    int32_t ddF_x = 1;
    int32_t i = 0;
    int32_t j = -1;

    do {
        while (f < 0) {
            ++i;
            f += (ddF_x += 2);
        }
        f += (ddF_y += 2);

        hLineLen(xc - i    , yc + r, i - j, col);
        hLineLen(xc - i    , yc - r, i - j, col);
        hLineLen(xc + j + 1, yc - r, i - j, col);
        hLineLen(xc + j + 1, yc + r, i - j, col);

        vLineLen(xc + r, yc + j + 1, i - j, col);
        vLineLen(xc + r, yc - i    , i - j, col);
        vLineLen(xc - r, yc - i    , i - j, col);
        vLineLen(xc - r, yc + j + 1, i - j, col);
        
        j = i;
    } while (i < --r);    
}

void GFX::fillCircle(int xc, int yc, int r, uint16_t col){
    if (!_lcd._scr.inited || r < 0) return;

    if (r == 0){
        putPixel(xc, yc, col);
        return;
    }

    int x = 0;
    int y = r;
    int d = 3 - (r << 1);

    hLine(xc - r, yc, xc + r, col);

    while (y >= x){
        if (x > 0){
            hLine(xc - y, yc + x, xc + y, col);
            hLine(xc - y, yc - x, xc + y, col);
        }
        if (y > x){
            hLine(xc - x, yc + y, xc + x, col);
            hLine(xc - x, yc - y, xc + x, col);
        }

        x++;
        if (d > 0){
            y--;
            d = d + ((x - y) << 2) + 10;
        } else {
            d = d + (x << 2) + 6;
        }
    }
}

void GFX::star(int x, int y, int radius, int sides, int ang, uint16_t col){
    if (sides < 2) return; // минимум 2 "луча"

    int points = sides * 2; // удвоенное количество вершин (внешние+внутренние)
    int* vx = new int[points];
    int* vy = new int[points];

    int angleStep = 360 / points;

    for (int i = 0; i < points; i++){
        int r = (i % 2 == 0) ? radius : radius / 2; // чётные - внешний радиус, нечётные - внутренний
        int angle = ang + i * angleStep;
        _lcd.LUT(vx[i], vy[i], x, y, r, angle);
    }

    // соединяем вершины линиями
    for (int i = 0; i < points; i++){
        int x1 = vx[i],     y1 = vy[i];
        int x2 = vx[(i+1)%points], y2 = vy[(i+1)%points];
        line(x1, y1, x2, y2, col);
    }

    delete[] vx;
    delete[] vy;
}

void GFX::fillStar(int x, int y, int radius, int sides, int rotation, uint16_t col){
    if (sides < 2) return;

    int points = sides * 2;
    int* vx = new int[points];
    int* vy = new int[points];

    int angleStep = 360 / points;

    for (int i = 0; i < points; i++){
        int r = (i % 2 == 0) ? radius : radius / 2;
        int angle = rotation + i * angleStep;
        _lcd.LUT(vx[i], vy[i], x, y, r, angle);
    }

    // используем тот же scanline, что и в fillPolygon
    int yMin = vy[0], yMax = vy[0];
    for (int i = 1; i < points; i++){
        if (vy[i] < yMin) yMin = vy[i];
        if (vy[i] > yMax) yMax = vy[i];
    }

    for (int yCur = yMin; yCur <= yMax; yCur++){
        int intersections[points];
        int n = 0;

        for (int i = 0; i < points; i++){
            int x1 = vx[i], y1 = vy[i];
            int x2 = vx[(i + 1) % points], y2 = vy[(i + 1) % points];

            if ((yCur >= y1 && yCur < y2) || (yCur >= y2 && yCur < y1)){
                int xInt = x1 + (yCur - y1) * (x2 - x1) / (y2 - y1);
                intersections[n++] = xInt;
            }
        }

        // сортируем пересечения
        for (int i = 0; i < n-1; i++){
            for (int j = i+1; j < n; j++){
                if (intersections[i] > intersections[j])
                    std::swap(intersections[i], intersections[j]);
            }
        }

        // рисуем горизонтальные линии
        for (int i = 0; i < n; i += 2){
            if (i+1 < n){
                hLine(intersections[i], yCur, intersections[i+1], col);
            }
        }
    }

    delete[] vx;
    delete[] vy;
}

void GFX::blur() {
    auto& s = _lcd._scr;
    if (!s.inited) return;

    int width  = _lcd.Width();
    int height = _lcd.Height();

    if (s.usePal || s.bpp == _8BIT){
        uint8_t* buf = s.bLine8[0];

        for (int y = 0; y < height - 1; y++) {
            uint8_t* line0 = buf + y * width;
            uint8_t* line1 = buf + (y + 1) * width;

            for (int x = 1; x < width - 1; x++) {
                uint8_t c0 = line0[x];
                uint8_t c1 = line1[x];
                uint8_t c2 = line1[x - 1];
                uint8_t c3 = line1[x + 1];

                int r = (R8(c0) + R8(c1) + ((R8(c2) + R8(c3)) >> 1)) / 3;
                int g = (G8(c0) + G8(c1) + ((G8(c2) + G8(c3)) >> 1)) / 3;
                int b = (B8(c0) + B8(c1) + ((B8(c2) + B8(c3)) >> 1)) / 3;

                line0[x] = (r << 5) | (g << 2) | b;
            }
        }
    } else {
        uint16_t* buf = s.bLine16[0];

        for (int y = 0; y < height - 1; y++) {
            uint16_t* line0 = buf + y * width;
            uint16_t* line1 = buf + (y + 1) * width;

            for (int x = 1; x < width - 1; x++) {
                uint16_t c0 = line0[x];
                uint16_t c1 = line1[x];
                uint16_t c2 = line1[x - 1];
                uint16_t c3 = line1[x + 1];

                int r = (R16(c0) + R16(c1) + ((R16(c2) + R16(c3)) >> 1)) / 3;
                int g = (G16(c0) + G16(c1) + ((G16(c2) + G16(c3)) >> 1)) / 3;
                int b = (B16(c0) + B16(c1) + ((B16(c2) + B16(c3)) >> 1)) / 3;

                line0[x] = (r << 11) | (g << 5) | b;
            }
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

            if (s.usePal || s.bpp == _8BIT){
                uint8_t r = (v >> 5);
                uint8_t g = (v >> 5);
                uint8_t b = (v >> 6);

                putPixel(cx + x, cy + y,             (uint8_t)(r << 5));
                putPixel(cx + x, cy + y + bandH,     (uint8_t)(g << 2));
                putPixel(cx + x, cy + y + bandH * 2, (uint8_t)b);
            } else {
                uint16_t r = (uint16_t)(v >> 3);
                uint16_t g = (uint16_t)(v >> 2);
                uint16_t b = (uint16_t)(v >> 3);

                putPixel(cx + x, cy + y,             (r << 11));
                putPixel(cx + x, cy + y + bandH,     (g << 5));
                putPixel(cx + x, cy + y + bandH * 2, b);
            }
        }
    }

    // ---- белая рамка вокруг блока ----
    rect(cx, cy, cx + rectW - 1, cy + rectH - 1, 0xFFFF);
}

void GFX::lineAA(float x0, float y0, float x1, float y1, uint16_t col) {
    auto& s = _lcd._scr;
    if (!s.inited) return;

    // AA пока только для RGB565
    if (s.usePal || s.bpp == _8BIT) {
        line((int)x0, (int)y0, (int)x1, (int)y1, col);
        return;
    }

    bool steep = fabsf(y1 - y0) > fabsf(x1 - x0);

    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }

    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    float dx = x1 - x0;
    float dy = y1 - y0;

    if (dx == 0.0f) {
        line((int)x0, (int)y0, (int)x1, (int)y1, col);
        return;
    }

    float gradient = dy / dx;

    // first endpoint
    float xend = roundf(x0);
    float yend = y0 + gradient * (xend - x0);
    float xgap = rfpart(x0 + 0.5f);

    int xpxl1 = (int)xend;
    int ypxl1 = (int)floorf(yend);

    uint8_t a1 = (uint8_t)(rfpart(yend) * xgap * 255.0f);
    uint8_t a2 = (uint8_t)( fpart(yend) * xgap * 255.0f);

    if (steep) {
        PutPixelAlpha(ypxl1,     xpxl1, col, a1);
        PutPixelAlpha(ypxl1 + 1, xpxl1, col, a2);
    } else {
        PutPixelAlpha(xpxl1, ypxl1,     col, a1);
        PutPixelAlpha(xpxl1, ypxl1 + 1, col, a2);
    }

    float intery = yend + gradient;

    // second endpoint
    xend = roundf(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = fpart(x1 + 0.5f);

    int xpxl2 = (int)xend;
    int ypxl2 = (int)floorf(yend);

    a1 = (uint8_t)(rfpart(yend) * xgap * 255.0f);
    a2 = (uint8_t)( fpart(yend) * xgap * 255.0f);

    if (steep) {
        PutPixelAlpha(ypxl2,     xpxl2, col, a1);
        PutPixelAlpha(ypxl2 + 1, xpxl2, col, a2);
    } else {
        PutPixelAlpha(xpxl2, ypxl2,     col, a1);
        PutPixelAlpha(xpxl2, ypxl2 + 1, col, a2);
    }

    // main loop
    if (steep) {
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
            int y = (int)floorf(intery);

            PutPixelAlpha(y,     x, col, (uint8_t)(rfpart(intery) * 255.0f));
            PutPixelAlpha(y + 1, x, col, (uint8_t)( fpart(intery) * 255.0f));

            intery += gradient;
        }
    } else {
        for (int x = xpxl1 + 1; x < xpxl2; x++) {
            int y = (int)floorf(intery);

            PutPixelAlpha(x, y,     col, (uint8_t)(rfpart(intery) * 255.0f));
            PutPixelAlpha(x, y + 1, col, (uint8_t)( fpart(intery) * 255.0f));

            intery += gradient;
        }
    }
}

void GFX::lineAAThick(float x0, float y0, float x1, float y1, int thick, uint16_t col) {
    if (thick <= 1) {
        lineAA(x0, y0, x1, y1, col);
        return;
    }

    float dx = x1 - x0;
    float dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len <= 0.0f) return;

    float nx = -dy / len;
    float ny =  dx / len;

    int r = thick / 2;

    for (int i = -r; i <= r; i++) {
        lineAA(
            x0 + nx * i,
            y0 + ny * i,
            x1 + nx * i,
            y1 + ny * i,
            col
        );
    }
}