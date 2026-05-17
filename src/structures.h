#pragma once

#define _8BIT   8
#define _16BIT  16
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
    //void*       fb2     = nullptr;
    //void*       fb3     = nullptr;
    //void*       fb4     = nullptr;
    void*       bgBuf   = nullptr;
    uint16_t**  fLine16 = nullptr;
    uint16_t**  bLine16 = nullptr;
    uint8_t**   fLine8  = nullptr;
    uint8_t**   bLine8  = nullptr;
    uint16_t*   pal     = nullptr; 

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
    uint8_t r = R8(c); // 3 bit
    uint8_t g = G8(c); // 3 bit
    uint8_t b = B8(c); // 2 bit

    uint16_t r5 = (r << 2) | (r >> 1);              // 3 -> 5
    uint16_t g6 = (g << 3) | g;                     // 3 -> 6
    uint16_t b5 = (b << 3) | (b << 1) | (b >> 1);   // 2 -> 5

    return (r5 << 11) | (g6 << 5) | b5;
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