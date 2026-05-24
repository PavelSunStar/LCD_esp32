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
    #include "drivers/esp_async_fbcpy.h"
    #include "esp_cache.h"
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
            //Init LUT
            for (int i = 0; i < LUT_SIZE; ++i) {
                sinLUT[i] = (int16_t)(1000.0 * sin(i * DEG_TO_RAD));
                cosLUT[i] = (int16_t)(1000.0 * cos(i * DEG_TO_RAD));
            }  

            #if IS_P4
                ppaFillInit();
                ppaCopyInit();

                initFBCopy();
            #endif                
        }

        //Screen 
        int BPP()       { return (_scr.usePal ? _8BIT : _scr.bpp); }
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

        void cls(uint16_t col = 0);
        inline void cls(uint8_t r, uint8_t g, uint8_t b);
        bool init(Screen_mode m = Mode640x480, bool usePal = false, uint8_t bpp = _16BIT, bool dBuff = false);// { return _drv.init(); } 
        void setViewport(int x0, int y0, int x1, int y1);
        void swap();
        //void flush(int fix) { _drv.flush(fix); };

        bool initBG();
        void scrToBg();
        void bgToScr();
        void scrToScr();

        void setPal(uint8_t index, uint16_t col);
        void setPal(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

        void LUT(int &x, int &y, int xx, int yy, int len, int angle);
        int xLUT(int x, int len, int angle);
        int yLUT(int y, int len, int angle);         

    private:
        int16_t sinLUT[LUT_SIZE];
        int16_t cosLUT[LUT_SIZE];

        esp_err_t err;
        friend class MIPI_Driver;
        friend class VGA_Driver;
        friend class LCD_Driver;

        friend class GFX;
        friend class Sprite;
        friend class Font_def;

        #if defined(LCD_DRIVER_MIPI)
            MIPI_Driver _drv;
        #elif defined(LCD_DRIVER_VGA)
            VGA_Driver _drv;
        #elif defined(LCD_DRIVER_LCD)
            LCD_Driver _drv;
        #endif

        #ifdef IS_P4
            bool _ppaFillInited = false;
            bool _ppaCopyInited = false;
            bool _fbCopyInited  = false;

            bool ppaFillInit(); 
            bool ppaCopyInit();

            ppa_client_handle_t         _ppaFill    = nullptr;
            ppa_srm_oper_config_t       _ppaCopyCfg = {};
            ppa_client_handle_t         _ppaCopy    = nullptr;

            bool initFBCopy();
            bool                        _fbInited           = false;
            esp_async_fbcpy_handle_t    _fbcpy              = NULL; // ← правильное имя
            SemaphoreHandle_t           _fbCopySemaphore    = NULL; // семафор для ожидания копирования
            bool asyncCopyRect(void* src, void* dst, int src_x, int src_y, int dst_x, int dst_y, int w, int h);
            static bool fbcpy_done_callback(esp_async_fbcpy_handle_t mcp, esp_async_fbcpy_event_data_t *event_data, void *cb_args);
        #endif

        bool                _semInited      = false;
        SemaphoreHandle_t   _sem_vsync_end  = nullptr;
        SemaphoreHandle_t   _sem_gui_ready  = nullptr;

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

/*
    auto &p = _ppaCopyCfg;
    p.in.buffer = s.fb2;
    p.in.pic_w = s.width;
    p.in.pic_h = s.height;
    p.in.block_w = s.width;
    p.in.block_h = s.height;
    p.in.block_offset_x = 0;
    p.in.block_offset_y = 0;
    p.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565; 
    
    p.out.buffer = s.fb1;
    p.out.buffer_size = c.size;
    p.out.pic_w = c.OUT_W; 
    p.out.pic_h = c.OUT_H;
    p.out.block_offset_x = 0;
    p.out.block_offset_y = 0;
    p.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565; 
    
    p.rotation_angle = PPA_SRM_ROTATION_ANGLE_0;
    p.scale_x = 1.0f;
    p.scale_y = 1.0f;
    p.mirror_x = false;
    p.mirror_y = false;  
    p.rgb_swap = false;
    p.byte_swap = false;
    p.alpha_update_mode = PPA_ALPHA_NO_CHANGE;
    p.mode = PPA_TRANS_MODE_BLOCKING; 
*/
