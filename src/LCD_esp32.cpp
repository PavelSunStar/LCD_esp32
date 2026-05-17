#include "LCD_esp32.h"

#include "esp_private/esp_cache_private.h"

void* LCD_esp32::allocateMemory(size_t request, bool psram, size_t* outAligned){
    if (psram && !PSRAM_OK){
        Serial.println("Error: PSRAM not present...");
        return nullptr;
    }

    size_t cache_align = 0;
    uint32_t caps = psram ? (MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA) : MALLOC_CAP_DMA;

    if (esp_cache_get_alignment(caps, &cache_align) != ESP_OK || cache_align < 2) {
        Serial.println("allocateMemory: esp_cache_get_alignment failed");
        return nullptr;
    }

    size_t aligned_size = (request + cache_align - 1) & ~(cache_align - 1);

    Serial.printf("allocateMemory: req=%u aligned=%u caps=0x%X align=%u\n",
                  (unsigned)request, (unsigned)aligned_size,
                  (unsigned)caps, (unsigned)cache_align);

    void* buffer = heap_caps_aligned_calloc(cache_align, 1, aligned_size, caps);
    if (!buffer) {
        Serial.println("allocateMemory: heap_caps_aligned_calloc failed");
        return nullptr;
    }

    //if (psram) _psramAling = cache_align;
    //else       _sramAlign  = cache_align;

    if (outAligned) *outAligned = aligned_size;
    return buffer;
}

bool LCD_esp32::setBufferAddr(){
    auto& s = _scr;

    if (!s.fb0 || s.width <= 0 || s.height <= 0) {
        Serial.println("initBuffer: invalid screen buffer");
        return false;
    }

    if (s.dBuff && !s.fb1) {
        Serial.println("setBufferAddr: fb1 null");
        return false;
    }    

    if (s.usePal && (!s.fb0 || !s.fb1)){
        Serial.println("setBufferAddr: buffer's null");
        return false; 
    }

    s.fLine16 = nullptr;
    s.bLine16 = nullptr;
    s.fLine8  = nullptr;
    s.bLine8  = nullptr;    
    int skip = s.width;
    int lines = s.height;

    if (s.usePal){
        // Set default pal
        uint16_t* p = s.pal; 
        for (int i = 0; i < 256; i++){
            uint16_t col = rgb332to565((uint8_t)i);

            for (int n = 0; n < scaleMul[s.scale]; n++){
                *p++ = col;
            }
        }   
        
        s.fLine8 = (uint8_t**)malloc(sizeof(uint8_t*) * lines);
        s.bLine8 = (uint8_t**)malloc(sizeof(uint8_t*) * lines);
        if (!s.fLine8 || !s.bLine8) return false;

        uint8_t* buf0 = (uint8_t*) s.fb0;
        uint8_t* buf1 = (uint8_t*) s.fb1;
        for (int y = 0; y < lines; y++){
            s.fLine8[y] = buf0;
            s.bLine8[y] = buf1;
            buf0 += skip;
            buf1 += skip;
        }  
        
        s.bLine8 = s.fLine8;
        s.fb1 = s.fb0;         
    } else {
        if (s.bpp == _16BIT){
            if (s.dBuff){
                s.fLine16 = (uint16_t**)malloc(sizeof(uint16_t*) * lines);
                s.bLine16 = (uint16_t**)malloc(sizeof(uint16_t*) * lines);
                if (!s.fLine16 || !s.bLine16) return false;

                uint16_t* buf0 = (uint16_t*)s.fb0;
                uint16_t* buf1 = (uint16_t*) s.fb1;
                for (int y = 0; y < lines; y++){
                    s.fLine16[y] = buf0;
                    s.bLine16[y] = buf1;
                    buf0 += skip;
                    buf1 += skip;
                }
            } else {
                s.fLine16 = (uint16_t**)malloc(sizeof(uint16_t*) * lines);
                if (!s.fLine16) return false;

                uint16_t* buf0 = (uint16_t*)s.fb0;
                for (int y = 0; y < lines; y++){
                    s.fLine16[y] = buf0;
                    buf0 += skip;
                }
                s.bLine16 = s.fLine16;
                s.fb1 = s.fb0;
            }
        } else if (s.bpp == _8BIT) {
            if (s.dBuff){
                s.fLine8 = (uint8_t**)malloc(sizeof(uint8_t*) * lines);
                s.bLine8 = (uint8_t**)malloc(sizeof(uint8_t*) * lines);
                if (!s.fLine8 || !s.bLine8) return false;

                uint8_t* buf0 = (uint8_t*) s.fb0;
                uint8_t* buf1 = (uint8_t*) s.fb1;
                for (int y = 0; y < lines; y++){
                    s.fLine8[y] = buf0;
                    s.bLine8[y] = buf1;
                    buf0 += skip;
                    buf1 += skip;
                }
            } else {
                s.fLine8 = (uint8_t**)malloc(sizeof(uint8_t*) * lines);
                if (!s.fLine8) return false;

                uint8_t* buf0 = (uint8_t*)s.fb0;
                for (int y = 0; y < lines; y++){
                    s.fLine8[y] = buf0;
                    buf0 += skip;
                }
                s.bLine8 = s.fLine8;
                s.fb1 = s.fb0;
            }
        } else {
            Serial.println("initBuffer: invalid bpp");
            return false;
        }
    }

    Serial.println("Buffer addr set...Ok");
    return true;    
}

void LCD_esp32::setScreenDimentions(int width, int height){
    auto& s = _scr;
    s.width     = width >> s.scale;
    s.height    = height >> s.scale;
    s.maxX      = s.width - 1;
    s.maxY      = s.height - 1;
    s.cx        = s.width >> 1;
    s.cy        = s.height >> 1;
    s.shift     = (s.usePal || s.bpp == _16BIT) ? 1 : 0;
    s.lineSize  = s.width << s.shift;
    s.size      = s.width * s.height;
    s.fullSize  = s.lineSize * s.height; 
    setViewport(0, 0, _scr.maxX, _scr.maxY);       
}

void LCD_esp32::setViewport(int x0, int y0, int x1, int y1){
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);

    _scr.x0 = std::clamp(x0, 0, _scr.maxX);
    _scr.y0 = std::clamp(y0, 0, _scr.maxY);
    _scr.x1 = std::clamp(x1, 0, _scr.maxX);
    _scr.y1 = std::clamp(y1, 0, _scr.maxY);
}

void LCD_esp32::getAlignSize(){
    // Get align
    uint32_t caps;

    // SRAM DMA
    caps = MALLOC_CAP_DMA;
    if (esp_cache_get_alignment(caps, &_sramAlign) != ESP_OK || _sramAlign < 2) {
        Serial.println("SRAM alignment error");
    }

    // PSRAM DMA
    caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA;
    if (esp_cache_get_alignment(caps, &_psramAlign) != ESP_OK || _psramAlign < 2) {
        Serial.println("PSRAM alignment error");
    }    
}

#ifdef IS_P4
bool LCD_esp32::ppaFillInit(){
    if (_ppaFillInited) return true;

    ppa_client_config_t cfg = {};
    cfg.oper_type = PPA_OPERATION_FILL;
    cfg.max_pending_trans_num = 1; 
    err = ppa_register_client(&cfg, &_ppaFill);

    return (_ppaFillInited = (err == ESP_OK));
}
#endif

bool LCD_esp32::init(Screen_mode m, bool usePal, uint8_t bpp, bool dBuff){ 
    return _drv.init(m, usePal, bpp, dBuff); 
}

void LCD_esp32::cls(uint16_t col){
    uint8_t r = R16_888(col);
    uint8_t g = G16_888(col);
    uint8_t b = B16_888(col);
    cls(r, g, b);
}

void LCD_esp32::cls(uint8_t r, uint8_t g, uint8_t b){
    auto& s = _scr;
    if (!s.inited) return;

    #if IS_P4
        uint16_t col = rgb888to565(r, g, b);

        if (_ppaFill){
            ppa_fill_oper_config_t cfg = {};

            if (_scr.usePal && ((col >> 8) == (col & 0xFF))){
                cfg.out.fill_cm = PPA_FILL_COLOR_MODE_RGB565;

                cfg.out.buffer = s.fb1;
                cfg.out.buffer_size = s.fullSize >> 1;
                cfg.out.pic_w = s.width;
                cfg.out.pic_h = s.height >> 1;

                cfg.out.block_offset_x = 0;
                cfg.out.block_offset_y = 0;
                cfg.fill_block_w = s.width;
                cfg.fill_block_h = s.height >> 1;

                cfg.fill_argb_color.a = 255;
                cfg.fill_argb_color.r = r;
                cfg.fill_argb_color.g = g;
                cfg.fill_argb_color.b = b;  
                    
                cfg.mode = PPA_TRANS_MODE_BLOCKING;
                cfg.user_data = nullptr;   

                ppa_do_fill(_ppaFill, &cfg);
                return;
            } else if (s.bpp == _16BIT){
                cfg.out.fill_cm = PPA_FILL_COLOR_MODE_RGB565;

                cfg.out.buffer = s.fb1;
                cfg.out.buffer_size = s.fullSize;
                cfg.out.pic_w = s.width;
                cfg.out.pic_h = s.height;

                cfg.out.block_offset_x = 0;
                cfg.out.block_offset_y = 0;
                cfg.fill_block_w = s.width;
                cfg.fill_block_h = s.height;

                cfg.fill_argb_color.a = 255;
                cfg.fill_argb_color.r = r;
                cfg.fill_argb_color.g = g;
                cfg.fill_argb_color.b = b;  
                    
                cfg.mode = PPA_TRANS_MODE_BLOCKING;
                cfg.user_data = nullptr;   

                ppa_do_fill(_ppaFill, &cfg);
                return;
            }
        }
    #endif

    if (_scr.bpp == _16BIT){
        uint16_t* scr = (uint16_t*) s.fb1;

        int col = rgb888to565(r, g, b);
        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, col, s.fullSize);
        } else {
            int size = 0;
            uint16_t* cpy = scr;
            while (size++ < _scr.width) *scr++ = col;

            int dummy = 1; 
            int lines = _scr.maxY;  
            int copyBytes = _scr.lineSize;
            int offset = _scr.width;
        
            while (lines > 0){ 
                if (lines >= dummy){
                    memcpy(scr, cpy, copyBytes);
                    lines -= dummy;
                    scr += offset;
                    copyBytes <<= 1;
                    offset <<= 1;
                    dummy <<= 1;
                } else {
                    copyBytes =_scr.lineSize * lines;
                    memcpy(scr, cpy, copyBytes);
                    break;
                }
            }            
        }        
    } else {
        memset(s.fb1, rgb888to332(r, g, b), _scr.fullSize);
    }    
}

bool LCD_esp32::regSemaphore(){
    if (_scr.dBuff){
        _sem_vsync_end = xSemaphoreCreateBinary();
        _sem_gui_ready = xSemaphoreCreateBinary();

        if (!_sem_vsync_end || !_sem_gui_ready){
            Serial.println("Error: Semaphores init fail.");
            return (_semInited = false);
        }
    } else {
        _sem_vsync_end = nullptr;
        _sem_gui_ready = nullptr;

        Serial.println("Double buffer not selected.");
        return (_semInited = false);
    }

    Serial.println("Registered semaphores...Ok");
    return (_semInited = true);
}
  
void LCD_esp32::swap(){
    if (_scr.dBuff){
        if (!_sem_gui_ready || !_sem_vsync_end){
            Serial.println("swap: semaphore not initialized");
            return;
        }

        xSemaphoreGive(_sem_gui_ready);
        xSemaphoreTake(_sem_vsync_end, portMAX_DELAY);
    }

    updateFPS();
}

void LCD_esp32::updateFPS(){
    uint64_t now = millis();
    _frameCount++;

    if (now - _fpsStartTime >= 1000) {
        _fps = _frameCount * 1000.0f / (now - _fpsStartTime);
        _frameCount = 0;
        _fpsStartTime = now;
    }
}

void LCD_esp32::setPal(uint8_t index, uint16_t col){
    if (_scr.scale == 0){ 
        ((uint16_t*)_scr.pal)[index] = col;
    } else if (_scr.scale == 1){
        uint32_t color = ((uint32_t)col << 16) | col;
        ((uint32_t*)_scr.pal)[index] = color;
    } else {
        uint64_t c = ((uint64_t)col << 48) |
                     ((uint64_t)col << 32) |
                     ((uint64_t)col << 16) |
                     ((uint64_t)col);

        ((uint64_t*)_scr.pal)[index] = c;
    }
}

void LCD_esp32::scrToScr(){
    if (!_scr.inited || !_scr.dBuff) return;
    memcpy(_scr.fb0, _scr.fb1, _scr.fullSize);
}