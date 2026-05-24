#pragma once
#include <math.h>
#include <stdint.h>

struct Affine2D{
    float a, b, c;
    float d, e, f;
};

struct Affine2DInv{
    int32_t a, b, c;
    int32_t d, e, f;
};

struct RectBounds{
    float sx, sy;
    float ex, ey;
};

static constexpr int   FP_SHIFT     = 16;
static constexpr int   FP_ONE       = 1 << FP_SHIFT;
static constexpr int   HALF         = 1 << (FP_SHIFT - 1);
static constexpr float deg_to_rad   = 0.01745329251994329577f; // π / 180
static inline    float angle(float deg)         { return (fmodf(deg, 360.0f) * deg_to_rad);}
static constexpr float percentTo(uint16_t p)    { return p * 0.01f; }

namespace Matrix {
    Affine2D identity();
    Affine2D translation(float tx, float ty);
    Affine2D scale(float sx, float sy);
    Affine2D rotate(float rad);
    Affine2D shear(float sx, float sy);

    Affine2D multiply(const Affine2D& m1, const Affine2D& m2);
    bool invert(Affine2DInv &out, const Affine2D &m);  

    void forward(const Affine2D& m, float x, float y, float& outx, float& outy);
    RectBounds bounds(const Affine2D& m, float imgW, float imgH);

    Affine2D make(float dst_x, float dst_y, float src_x, float src_y, float angle, float zoom_x, float zoom_y);
    Affine2D make(float dst_x, float dst_y, float src_x, float src_y, float angle, float zoom_x, float zoom_y, float shear_x, float shear_y);
}

//1494
// 1676 void LGFXBase::push_image_affine_aa(const float* matrix, pixelcopy_t* pc, pixelcopy_t* pc2)