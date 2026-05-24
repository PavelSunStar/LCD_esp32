#pragma once

#define _8BIT   8
#define _16BIT  16
#define LUT_SIZE 360
#define ALIGN_UP(size, align)(((size) + ((align) - 1)) & ~((align) - 1))

enum Screen_mode{
    Mode160x120,
    Mode160x200,
    Mode240x128,
    Mode256x192,
    Mode320x200,
    Mode320x240,
    Mode360x400,
    Mode400x300,
    Mode512x384,
    Mode512x480,
    Mode512x512,
    Mode528x400,
    Mode528x480,
    Mode640x200,
    Mode640x350,
    Mode640x400,
    Mode640x480,
    Mode720x348,
    Mode720x350,
    Mode720x360,
    Mode720x400,
    Mode720x480,
    Mode720x512,
    Mode720x540,
    Mode752x410,
    Mode800x560,
    Mode800x600,
};

static const int scaleMul[] = {1, 2, 4};

struct Screen{
    bool        inited  = false;
    bool        dBuff   = false;
    bool        usePal  = false;
    uint8_t     scale   = 0;

    // Buffer
    void*       fb0     = nullptr; 
    void*       fb1     = nullptr;
    uint16_t**  fLine16 = nullptr;
    uint16_t**  bLine16 = nullptr;
    uint8_t**   fLine8  = nullptr;
    uint8_t**   bLine8  = nullptr;
    uint16_t*   pal     = nullptr; 

    void*       bg      = nullptr;
    uint8_t**   bgLine8 = nullptr;
    uint16_t**  bgLine16 = nullptr;    

    // Screen config
    uint8_t     bpp, shift;
	int         width, height;
	int         maxX, maxY;
	int         cx, cy;
    int         lineSize;
    int         size, fullSize;
    int         sizeAlign;
    int         fullSizeAlign;
    
    // viewport
    int         x0 = 0, x1 = 0;
    int         y0 = 0, y1 = 0;

    // Backgroung
    int         bgWidth     = 0; 
    int         bgHeight    = 0;
};

/*
    switch (c.rotation) {
        case 1:
            c.rot = PPA_SRM_ROTATION_ANGLE_90;
            break;

        case 2:
            c.rot = PPA_SRM_ROTATION_ANGLE_180;
            break;

        case 3:
            c.rot = PPA_SRM_ROTATION_ANGLE_270;
            break;

        default:
            c.rot = PPA_SRM_ROTATION_ANGLE_0;
            break;
    }
*/

struct Image{
    int width, height;
    int maxX, maxY;
    int cx, cy;
    int lineSize;
    int size, fullSize;
    uint32_t offset;
    uint32_t offsetLine;
};

static const uint16_t TABLE_332to565[] = {
    0x0000, 0x000a, 0x0015, 0x001f, 0x0120, 0x012a, 0x0135, 0x013f, 0x0240,
    0x024a, 0x0255, 0x025f, 0x0360, 0x036a, 0x0375, 0x037f, 0x0480, 0x048a,
    0x0495, 0x049f, 0x05a0, 0x05aa, 0x05b5, 0x05bf, 0x06c0, 0x06ca, 0x06d5,
    0x06df, 0x07e0, 0x07ea, 0x07f5, 0x07ff, 0x2000, 0x200a, 0x2015, 0x201f,
    0x2120, 0x212a, 0x2135, 0x213f, 0x2240, 0x224a, 0x2255, 0x225f, 0x2360,
    0x236a, 0x2375, 0x237f, 0x2480, 0x248a, 0x2495, 0x249f, 0x25a0, 0x25aa,
    0x25b5, 0x25bf, 0x26c0, 0x26ca, 0x26d5, 0x26df, 0x27e0, 0x27ea, 0x27f5,
    0x27ff, 0x4800, 0x480a, 0x4815, 0x481f, 0x4920, 0x492a, 0x4935, 0x493f,
    0x4a40, 0x4a4a, 0x4a55, 0x4a5f, 0x4b60, 0x4b6a, 0x4b75, 0x4b7f, 0x4c80,
    0x4c8a, 0x4c95, 0x4c9f, 0x4da0, 0x4daa, 0x4db5, 0x4dbf, 0x4ec0, 0x4eca,
    0x4ed5, 0x4edf, 0x4fe0, 0x4fea, 0x4ff5, 0x4fff, 0x6800, 0x680a, 0x6815,
    0x681f, 0x6920, 0x692a, 0x6935, 0x693f, 0x6a40, 0x6a4a, 0x6a55, 0x6a5f,
    0x6b60, 0x6b6a, 0x6b75, 0x6b7f, 0x6c80, 0x6c8a, 0x6c95, 0x6c9f, 0x6da0,
    0x6daa, 0x6db5, 0x6dbf, 0x6ec0, 0x6eca, 0x6ed5, 0x6edf, 0x6fe0, 0x6fea,
    0x6ff5, 0x6fff, 0x9000, 0x900a, 0x9015, 0x901f, 0x9120, 0x912a, 0x9135,
    0x913f, 0x9240, 0x924a, 0x9255, 0x925f, 0x9360, 0x936a, 0x9375, 0x937f,
    0x9480, 0x948a, 0x9495, 0x949f, 0x95a0, 0x95aa, 0x95b5, 0x95bf, 0x96c0,
    0x96ca, 0x96d5, 0x96df, 0x97e0, 0x97ea, 0x97f5, 0x97ff, 0xb000, 0xb00a,
    0xb015, 0xb01f, 0xb120, 0xb12a, 0xb135, 0xb13f, 0xb240, 0xb24a, 0xb255,
    0xb25f, 0xb360, 0xb36a, 0xb375, 0xb37f, 0xb480, 0xb48a, 0xb495, 0xb49f,
    0xb5a0, 0xb5aa, 0xb5b5, 0xb5bf, 0xb6c0, 0xb6ca, 0xb6d5, 0xb6df, 0xb7e0,
    0xb7ea, 0xb7f5, 0xb7ff, 0xd800, 0xd80a, 0xd815, 0xd81f, 0xd920, 0xd92a,
    0xd935, 0xd93f, 0xda40, 0xda4a, 0xda55, 0xda5f, 0xdb60, 0xdb6a, 0xdb75,
    0xdb7f, 0xdc80, 0xdc8a, 0xdc95, 0xdc9f, 0xdda0, 0xddaa, 0xddb5, 0xddbf,
    0xdec0, 0xdeca, 0xded5, 0xdedf, 0xdfe0, 0xdfea, 0xdff5, 0xdfff, 0xf800,
    0xf80a, 0xf815, 0xf81f, 0xf920, 0xf92a, 0xf935, 0xf93f, 0xfa40, 0xfa4a,
    0xfa55, 0xfa5f, 0xfb60, 0xfb6a, 0xfb75, 0xfb7f, 0xfc80, 0xfc8a, 0xfc95,
    0xfc9f, 0xfda0, 0xfdaa, 0xfdb5, 0xfdbf, 0xfec0, 0xfeca, 0xfed5, 0xfedf,
    0xffe0, 0xffea, 0xfff5, 0xffff
};

// ======================================================
// 8 bit color - RGB332
// ======================================================
inline uint8_t RGB8(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 5) << 5) |
           ((g >> 5) << 2) |
           ( b >> 6);
}

inline uint8_t R8(uint8_t c) {
    return (c >> 5) & 0x07;
}

inline uint8_t G8(uint8_t c) {
    return (c >> 2) & 0x07;
}

inline uint8_t B8(uint8_t c) {
    return c & 0x03;
}

// RGB332 raw bits -> RGB888 channel
inline uint8_t R8_888(uint8_t c) {
    uint8_t r = R8(c);
    return (r << 5) | (r << 2) | (r >> 1);   // 3 -> 8
}

inline uint8_t G8_888(uint8_t c) {
    uint8_t g = G8(c);
    return (g << 5) | (g << 2) | (g >> 1);   // 3 -> 8
}

inline uint8_t B8_888(uint8_t c) {
    uint8_t b = B8(c);
    return (b << 6) | (b << 4) | (b << 2) | b; // 2 -> 8
}

// ======================================================
// 16 bit color - RGB565
// ======================================================

inline uint16_t RGB16(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint16_t)(r >> 3) << 11) |
           ((uint16_t)(g >> 2) << 5)  |
           ((uint16_t)(b >> 3));
}

// raw 5/6/5 channel values
inline uint8_t R16(uint16_t c) {
    return (c >> 11) & 0x1F;
}

inline uint8_t G16(uint16_t c) {
    return (c >> 5) & 0x3F;
}

inline uint8_t B16(uint16_t c) {
    return c & 0x1F;
}

// RGB565 -> RGB888 channel
inline uint8_t R16_888(uint16_t c) {
    uint8_t r = R16(c);
    return (r << 3) | (r >> 2);              // 5 -> 8
}

inline uint8_t G16_888(uint16_t c) {
    uint8_t g = G16(c);
    return (g << 2) | (g >> 4);              // 6 -> 8
}

inline uint8_t B16_888(uint16_t c) {
    uint8_t b = B16(c);
    return (b << 3) | (b >> 2);              // 5 -> 8
}

// ======================================================
// RGB888 <-> RGB565
// ======================================================

inline uint16_t rgb888to565(uint8_t r, uint8_t g, uint8_t b) {
    return RGB16(r, g, b);
}

inline void rgb565to888(uint16_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = R16_888(c);
    g = G16_888(c);
    b = B16_888(c);
}

// ======================================================
// RGB888 <-> RGB332
// ======================================================

inline uint8_t rgb888to332(uint8_t r, uint8_t g, uint8_t b) {
    return RGB8(r, g, b);
}

inline void rgb332to888(uint8_t c, uint8_t& r, uint8_t& g, uint8_t& b) {
    r = R8_888(c);
    g = G8_888(c);
    b = B8_888(c);
}

// ======================================================
// RGB332 <-> RGB565
// ======================================================

inline uint16_t rgb332to565(uint8_t c) {
    /*
    uint8_t r = R8(c); // 3 bit
    uint8_t g = G8(c); // 3 bit
    uint8_t b = B8(c); // 2 bit

    uint16_t r5 = (r << 2) | (r >> 1);              // 3 -> 5
    uint16_t g6 = (g << 3) | g;                     // 3 -> 6
    uint16_t b5 = (b << 3) | (b << 1) | (b >> 1);   // 2 -> 5

    return (r5 << 11) | (g6 << 5) | b5;
    */
    return TABLE_332to565[c];
}

inline uint8_t rgb565to332(uint16_t c) {
    return ((c >> 8) & 0xE0) |   // R: 5 bit -> 3 bit
           ((c >> 6) & 0x1C) |   // G: 6 bit -> 3 bit
           ((c >> 3) & 0x03);    // B: 5 bit -> 2 bit
}

// ======================================================
// Optional helpers
// ======================================================

inline uint16_t swap565(uint16_t c) {
    return (c >> 8) | (c << 8);
}

static inline uint16_t blend565(uint16_t bg, uint16_t fg, uint8_t a){
    uint32_t rb = (((fg & 0xF81F) * a + (bg & 0xF81F) * (255 - a)) >> 8) & 0xF81F;
    uint32_t g  = (((fg & 0x07E0) * a + (bg & 0x07E0) * (255 - a)) >> 8) & 0x07E0;
    return rb | g;
}

// a	foreground color (верхний цвет)
// b	background color (нижний цвет)
// c    alpha / прозрачность
inline uint16_t calcAlpha(uint16_t a, uint16_t b, uint8_t c) {
    uint32_t alpha = c >> 3;

    uint32_t fg = (a | (a << 16)) & 0x07E0F81F;
    uint32_t bg = (b | (b << 16)) & 0x07E0F81F;

    bg += ((fg - bg) * alpha) >> 5;
    bg &= 0x07E0F81F;

    return (uint16_t)(bg | (bg >> 16));
}

static inline float fpart(float x) {
    return x - floorf(x);
}

static inline float rfpart(float x) {
    return 1.0f - fpart(x);
}

