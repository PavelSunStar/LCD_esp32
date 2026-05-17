#include <LCD_esp32.h>
#include "GFX.h"
#include <math.h>

LCD_esp32 lcd;
GFX gfx(lcd);

int R = 24;

int bx1 = 80,  by1 = 55,  vx1 = 1;
int bx2 = 180, by2 = 70,  vx2 = -1;

float fy1 = by1, speedY1 = -2.0f;
float fy2 = by2, speedY2 = -4.0f;

float ang1 = 0;
float ang2 = 1.5f;

uint16_t RGB565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

void setPalettes() {
    lcd.setPal(0, RGB565(170,170,170));
    lcd.setPal(1, RGB565(90,90,90));
    lcd.setPal(2, RGB565(40,40,40));

    for (int i = 0; i < 32; i++) {
        int v = i * 8;

        // ball 1: red / white
        lcd.setPal(32 + i, RGB565(v, 0, 0));
        lcd.setPal(96 + i, RGB565(v, v, v));

        // ball 2: blue / yellow
        lcd.setPal(128 + i, RGB565(0, 0, v));
        lcd.setPal(160 + i, RGB565(v, v, 0));
    }
}

void drawGrid() {
    lcd.cls(0xf0f0);

    for (int y = 0; y <= lcd.MaxY(); y += 10) gfx.hLine(0, y, lcd.MaxX(), 1);
    for (int x = 0; x <= lcd.MaxX(); x += 10) gfx.vLine(x, 0, lcd.MaxY(), 1);
}

void drawShadow(int bx, int by, int r) {
    int sy = by + r + 6;
    int sw = r + 10;
    int sh = 7;

    for (int y = -sh; y <= sh; y++) {
        for (int x = -sw; x <= sw; x++) {
            if ((x * x * 100) / (sw * sw) + (y * y * 100) / (sh * sh) <= 100) {
                gfx.putPixel(bx + x + 8, sy + y, 2);
            }
        }
    }
}

void drawBall(int bx, int by, int r, float ang, uint8_t col1, uint8_t col2){
    float ca = cosf(ang);
    float sa = sinf(ang);

    float ca2 = cosf(ang * 0.65f);
    float sa2 = sinf(ang * 0.65f);

    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            int rr = x * x + y * y;
            if (rr > r * r) continue;

            float nx = (float)x / r;
            float ny = (float)y / r;
            float nz = sqrtf(1.0f - nx * nx - ny * ny);

            float rx = nx * ca - nz * sa;
            float rz = nx * sa + nz * ca;
            float ry = ny * ca2 - rz * sa2;

            int tx = (int)floorf((rx + 1.0f) * 5.0f);
            int ty = (int)floorf((ry + 1.0f) * 5.0f);

            bool red = ((tx + ty) & 1);

            float light = nx * -0.35f + ny * -0.45f + nz * 0.95f;
            if (light < 0.10f) light = 0.10f;
            if (light > 1.0f)  light = 1.0f;

            int shade = (int)(light * 31.0f);
            //uint8_t col = red ? (32 + shade) : (96 + shade);
            uint8_t col = red ? (col1 + shade) : (col2 + shade);
            gfx.putPixel(bx + x, by + y, col);
        }
    }
}

void updateBall(int &bx, int &by, int &vx, float &fy, float &speedY, float &ang, float jump) {
    fy += speedY;
    speedY += 0.05f;

    if (fy + R >= lcd.MaxY() - 20) {
        fy = lcd.MaxY() - 20 - R;
        speedY = jump;
    }

    by = (int)fy;

    bx += vx;
    if (bx - R <= 0 || bx + R >= lcd.MaxX()) vx = -vx;

    //ang += 0.1f;
    ang += (vx > 0) ? 0.1f : -0.1f;
    //ang2 -= 0.12f;
}

void setup() {
    Serial.begin(115200);

    lcd.init(Mode320x240, true, _8BIT, true);
    setPalettes();
}

void loop() {
    drawGrid();

    drawShadow(bx1, by1, R);
    drawShadow(bx2, by2, R);

    //drawBall(bx1, by1, R, ang1);
    //drawBall(bx2, by2, R, ang2);
    drawBall(bx1, by1, R, ang1, 32, 96);    // red/white
    drawBall(bx2, by2, R, ang2, 128, 160);  // blue/yellow
    updateBall(bx1, by1, vx1, fy1, speedY1, ang1, -3.8f);
    updateBall(bx2, by2, vx2, fy2, speedY2, ang2, -3.5f);

    lcd.swap();
}