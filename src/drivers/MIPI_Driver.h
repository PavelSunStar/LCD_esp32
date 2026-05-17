#pragma once

#ifdef LCD_DRIVER_MIPI

#include <Arduino.h>
#include "esp_lcd_panel_ops.h"

class MIPI_Driver {
    public:
        bool init();
        //void flush();

    private:
        //esp_lcd_panel_handle_t _panel = nullptr;
};

#endif