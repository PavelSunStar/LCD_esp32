#include "Matrix.h"

namespace Matrix {
    Affine2D identity(){
        return {1.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f};
    }

    Affine2D translation(float tx, float ty){
        return {1.0f, 0.0f, tx,
                0.0f, 1.0f, ty};
    }

    Affine2D scale(float sx, float sy){
        return {sx,   0.0f, 0.0f,
                0.0f, sy,   0.0f};
    }

    Affine2D rotate(float rad){
        float s = sinf(rad);
        float c = cosf(rad);
        return {c, -s, 0.0f,
                s,  c, 0.0f};
    }

    Affine2D shear(float sx, float sy){
        return {1.0f, sx,   0.0f,
                sy,   1.0f, 0.0f};
    }

    Affine2D multiply(const Affine2D& m1, const Affine2D& m2){
        return {
            m1.a * m2.a + m1.b * m2.d,
            m1.a * m2.b + m1.b * m2.e,
            m1.a * m2.c + m1.b * m2.f + m1.c,

            m1.d * m2.a + m1.e * m2.d,
            m1.d * m2.b + m1.e * m2.e,
            m1.d * m2.c + m1.e * m2.f + m1.f
        };
    }

    void forward(const Affine2D& m, float x, float y, float& outx, float& outy){
        outx = m.a * x + m.b * y + m.c;
        outy = m.d * x + m.e * y + m.f;
    }

    static inline void updateBounds(float px, float py, float& sx, float& sy, float& ex, float& ey){
        if (px < sx) sx = px;
        if (py < sy) sy = py;
        if (px > ex) ex = px;
        if (py > ey) ey = py;
    }

    RectBounds bounds(const Affine2D& m, float imgW, float imgH){
        float sx, sy, ex, ey;
        float px, py;

        forward(m, 0.0f, 0.0f, px, py);
        sx = ex = px;
        sy = ey = py;

        forward(m, imgW, 0.0f, px, py);
        updateBounds(px, py, sx, sy, ex, ey);

        forward(m, 0.0f, imgH, px, py);
        updateBounds(px, py, sx, sy, ex, ey);

        forward(m, imgW, imgH, px, py);
        updateBounds(px, py, sx, sy, ex, ey);

        return {sx, sy, ex, ey};
    }

    bool invert(Affine2DInv &out, const Affine2D &m){
        float det = m.a * m.e - m.b * m.d;
        if (fabsf(det) < 1e-6f) return false;

        float d = (float)FP_ONE / det;
        out.a = (int32_t)roundf(d *  m.e);
        out.b = (int32_t)roundf(d * -m.b);
        out.c = (int32_t)roundf(d * (m.b * m.f - m.e * m.c));
        out.d = (int32_t)roundf(d * -m.d);
        out.e = (int32_t)roundf(d *  m.a);
        out.f = (int32_t)roundf(d * (m.d * m.c - m.a * m.f));

        return true;
    }
    
    Affine2D make(float dst_x, float dst_y, float src_x, float src_y, float angle, float zoom_x, float zoom_y){
        //const float rad   = fmodf(angle, 360.0f) * deg_to_rad;
        const float sin_f = sinf(angle); //sinf(rad);
        const float cos_f = cosf(angle); //cosf(rad);

        Affine2D m;
        m.a =  cos_f * zoom_x;
        m.b = -sin_f * zoom_y;
        m.c =  dst_x - src_x * m.a - src_y * m.b;
        m.d =  sin_f * zoom_x;
        m.e =  cos_f * zoom_y;
        m.f =  dst_y - src_x * m.d - src_y * m.e;

        return m;
    }  
    
    Affine2D make(float dst_x, float dst_y, float src_x, float src_y, float angle, float zoom_x, float zoom_y, float shear_x, float shear_y){
        const float rad = fmodf(angle, 360.0f) * deg_to_rad;

        Affine2D t0 = translation(-src_x, -src_y);
        Affine2D s  = scale(zoom_x, zoom_y);
        Affine2D sh = shear(shear_x, shear_y);
        Affine2D r  = rotate(rad);
        Affine2D t1 = translation(dst_x, dst_y);

        return multiply(t1, multiply(r, multiply(sh, multiply(s, t0))));
    }    
}
