#pragma once

#ifdef LCD_DRIVER_LCD

#include <Arduino.h>

class LCD_Driver {
    public:
        bool init();
        //void flush();
};

#endif