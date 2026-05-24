#pragma once

#include "../LCD_config.h"
#include <Arduino.h>
#include "structures.h"

#ifdef LCD_DRIVER_VGA
    #include "esp_lcd_panel_rgb.h"
/*        
    struct Mode {
        int pclk_hz;
        int hRes, hFront, hSync, hBack, hPol;
        int vRes, vFront, vSync, vBack, vPol;
    };
    inline Mode MODE640x480_60Hz =  {25000000, 640, 16, 96, 48, 1, 480, 10, 2, 33, 1};    
*/

    class LCD_esp32;

    class VGA_Driver{
        public:
            VGA_Driver(LCD_esp32 &lcd);
            ~VGA_Driver();

            bool init(Screen_mode m, bool usePal, uint8_t bpp, bool dBuff);
            //bool init(Mode m = MODE640x480_60Hz, uint8_t bpp = _16BIT, bool dBuff = false);
            //void flush();

        private:
            bool selectMode(Screen_mode m);
            int optimal_bounce_buffer_px();
            bool setRGBPanel(uint8_t panel_bpp);
            //bool regSemaphore();
            void regCallBack();
            bool initPanel();

            static bool IRAM_ATTR on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx);
            static bool IRAM_ATTR on_bounce_empty_p4(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx);
            static bool IRAM_ATTR on_bounce_empty_p4_pal(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx);

            esp_err_t                   err;
            esp_lcd_rgb_panel_config_t  panel_config = {};
            esp_lcd_panel_handle_t      panel_handle = nullptr;
           
            int _lastPos;
            int _lines;
            int _pixels;
            int _copyBytes;
            int _copyBytes2x;
            int _skip;
            int _bounceBufferSize_px;

            Mode _m = {};
            LCD_esp32 &_lcd;
    };

    
#endif

/*
esp_lcd_rgb_panel_config_t panel_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .data_width = (uint8_t)bits,           // 8 или 16
        .bits_per_pixel = (uint8_t)bits,
        .hsync_gpio_num = pinConfig.hSync,
        .vsync_gpio_num = pinConfig.vSync,
        .de_gpio_num = -1,                     // VGA обычно без DE
        .pclk_gpio_num = -1,                   // для VGA PCLK не нужен
        .disp_gpio_num = -1,
        .timings = {
            .pclk_hz = vgaMode.frequency,
            .h_res = vgaMode.hRes,
            .v_res = vgaMode.vRes,
            .hsync_pulse_width = vgaMode.hSync,
            .hsync_back_porch = vgaMode.hBack,
            .hsync_front_porch = vgaMode.hFront,
            .vsync_pulse_width = vgaMode.vSync,
            .vsync_back_porch = vgaMode.vBack,
            .vsync_front_porch = vgaMode.vFront,
            .flags = {
                .hsync_idle_pol = vgaMode.hPol ^ 1,
                .vsync_idle_pol = vgaMode.vPol ^ 1,
            }
        },
        .flags = {
            .fb_in_psram = usePsram,
        }
    };
*/