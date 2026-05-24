#pragma once

#include "../LCD_config.h"
#include <Arduino.h>
#include "structures.h"

#ifdef LCD_DRIVER_LCD
    #include "esp_lcd_panel_rgb.h"

    class LCD_esp32;

    class LCD_Driver {
        public:
            LCD_Driver(LCD_esp32 &lcd);
            ~LCD_Driver();

            bool init(Screen_mode m, bool usePal, uint8_t bpp, bool dBuff);
            void flush(int fix);
            
        private: 
            bool selectMode(Screen_mode m);
            bool setRGBPanel();
            bool initPanel();

            esp_err_t                   err;
            esp_lcd_rgb_panel_config_t  panel_config = {};
            esp_lcd_panel_handle_t      panel_handle = nullptr;

            bool _first = true;
            Mode _m = {};
            LCD_esp32 &_lcd;    
    };

#endif