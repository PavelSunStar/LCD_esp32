#pragma once

#include "LCD_config.h"
#include <Arduino.h>

#include "structures.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/semphr.h"

// ===============================
// TARGET CHECK
// ===============================

#if defined(CONFIG_IDF_TARGET_ESP32P4) || defined(ARDUINO_ESP32P4_DEV)
    #define IS_P4 1
#else
    #define IS_P4 0
#endif

// ===============================
// PSRAM CHECK
// ===============================

#ifdef BOARD_HAS_PSRAM
    #define PSRAM_OK 1
#else
    #define PSRAM_OK 0
#endif

// ===============================
// USER DRIVER SELECT
// ===============================

// Проверка: выбран ли хоть один драйвер
#if !defined(LCD_DRIVER_MIPI) && !defined(LCD_DRIVER_VGA) && !defined(LCD_DRIVER_LCD)
    #error "Select one video driver before #include <LCD_esp32.h>: LCD_DRIVER_MIPI, LCD_DRIVER_VGA or LCD_DRIVER_LCD"
#endif

// Проверка: нельзя выбрать несколько сразу
#if \
    (defined(LCD_DRIVER_MIPI) + defined(LCD_DRIVER_VGA) + defined(LCD_DRIVER_LCD)) != 1
    #error "Select only ONE video driver"
#endif

// ===============================
// DRIVER INCLUDES
// ===============================
#ifdef IS_P4
    #include "driver/ppa.h"
#endif

#if defined(LCD_DRIVER_MIPI)

    #if !IS_P4
        #error "MIPI driver works only on ESP32-P4"
    #endif

    #include "drivers/MIPI_Driver.h"

#elif defined(LCD_DRIVER_VGA)

    #include "drivers/VGA_Driver.h"

#elif defined(LCD_DRIVER_LCD)

    #include "drivers/LCD_Driver.h"

#endif

class LCD_esp32 {
    public:
        LCD_esp32() : _drv(*this){
            #if IS_P4
                ppaFillInit();
            #endif                
        }

        //Screen 
        int BPP()       { return _scr.bpp; }
        int Width()     { return _scr.width; }
        int Height()    { return _scr.height; }
        int MaxX()      { return _scr.maxX; }
        int MaxY()      { return _scr.maxY; }
        int CX()        { return _scr.cx; } 
        int CY()        { return _scr.cy; }

        //Screen viewport
        int vX1()       {return _scr.x0;}
        int vY1()       {return _scr.y0;}
        int vX2()       {return _scr.x1;}
        int vY2()       {return _scr.y1;}

        // FPS
        float FPS()         { return _fps; }
        uint32_t Timer()    { return _timer; }

        void cls(uint16_t col);
        void cls(uint8_t r, uint8_t g, uint8_t b);
        bool init(Screen_mode m = Mode640x480, bool usePal = false, uint8_t bpp = _16BIT, bool dBuff = false);// { return _drv.init(); } 
        void setViewport(int x0, int y0, int x1, int y1);
        void swap();

        void scrToScr();
        void setPal(uint8_t index, uint16_t col);

    private:
        esp_err_t err;
        friend class MIPI_Driver;
        friend class VGA_Driver;
        friend class LCD_Driver;

        friend class GFX;

        #if defined(LCD_DRIVER_MIPI)
            MIPI_Driver _drv;
        #elif defined(LCD_DRIVER_VGA)
            VGA_Driver _drv;
        #elif defined(LCD_DRIVER_LCD)
            LCD_Driver _drv;
        #endif

        bool _ppaFillInited = false;
        #ifdef IS_P4
            bool ppaFillInit();

            ppa_client_handle_t _ppaFill = nullptr;
            //ppa_srm_oper_config_t bounce_cfg = {};
            //ppa_client_handle_t bounce_srm = nullptr;            
        #endif

        bool                _semInited = false;
        SemaphoreHandle_t   _sem_vsync_end = nullptr;
        SemaphoreHandle_t   _sem_gui_ready = nullptr;

        Screen  _scr;
        Pins    _pins = defPins_P4;

        size_t  _sramAlign; 
        size_t  _psramAlign;

        void getAlignSize();
        void setScreenDimentions(int width, int height);
        void* allocateMemory(size_t request, bool psram = true, size_t* outAligned = nullptr);
        bool setBufferAddr();
        bool regSemaphore(); 
        void updateFPS();

        // FPS
        float               _fps = 0.0f;
        volatile uint32_t   _frameCount = 0;
        uint64_t            _fpsStartTime = 0;
        volatile uint32_t   _timer = 0;
};