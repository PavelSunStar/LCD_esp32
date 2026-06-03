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


/*
void LCD_esp32::bgToScr() {
    auto& s = _scr;
    if (!s.inited || !s.bg) return;

    void* buf = (s.usePal || s.bpp == _8BIT)
              ? (void*)s.bLine8[0]
              : (void*)s.bLine16[0];

    #if IS_P4
        if (_fbCopy && _fbCopyDone && !s.usePal && s.bpp == _16BIT) {
            esp_async_fbcpy_trans_desc_t t = {};

            t.src_buffer = s.bg;
            t.dst_buffer = buf;

            t.src_offset_x = 0;
            t.src_offset_y = 0;
            t.dst_offset_x = 0;
            t.dst_offset_y = 0;

            t.src_buffer_size_x = s.width;
            t.src_buffer_size_y = s.height;
            t.dst_buffer_size_x = s.width;
            t.dst_buffer_size_y = s.height;

            t.copy_size_x = s.width;
            t.copy_size_y = s.height;

            t.pixel_format_unique_id.color_space  = COLOR_SPACE_RGB;
            t.pixel_format_unique_id.pixel_format = COLOR_PIXEL_RGB565;

            // очистить старый done-семафор
            xSemaphoreTake(_fbCopyDone, 0);

            esp_err_t ret = esp_async_fbcpy(
                _fbCopy,
                &t,
                fbcopy_done_cb,
                _fbCopyDone
            );

            if (ret == ESP_OK) {
                // ждать, пока DMA реально закончит копирование
                xSemaphoreTake(_fbCopyDone, portMAX_DELAY);

                esp_cache_msync(
                    buf,
                    s.fullSize,
                    ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED
                );

                return;
            }

            Serial.printf("fbCopy error: %s\n", esp_err_to_name(ret));
        }
    #endif

    memcpy(buf, s.bg, s.fullSize);
}
*/
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
    s.shift     = (s.usePal || s.bpp == _8BIT) ? 0 : 1;
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

bool LCD_esp32::init(Screen_mode m, bool usePal, uint8_t bpp, bool dBuff){ 
    return _drv.init(m, usePal, bpp, dBuff); 
}

void LCD_esp32::cls(uint8_t r, uint8_t g, uint8_t b) {
    auto& s = _scr;
    if (!s.inited) return;

#if IS_P4
    if (_ppaFill) {
        ppa_fill_oper_config_t cfg = {};

        if (s.usePal || s.bpp == _8BIT) {

            // RGB332 -> duplicated byte -> RGB565
            uint8_t c = rgb888to332(r, g, b);
            uint16_t color = ((uint16_t)c << 8) | c;

            cfg.fill_argb_color.r = R16(color);
            cfg.fill_argb_color.g = G16(color);
            cfg.fill_argb_color.b = B16(color);

            cfg.out.buffer = (void*)s.bLine8[0];
            cfg.fill_block_w = s.width >> 1;
            cfg.out.pic_w = s.width >> 1;

        } else {

            cfg.fill_argb_color.r = r;
            cfg.fill_argb_color.g = g;
            cfg.fill_argb_color.b = b;

            cfg.out.buffer = (void*)s.bLine16[0];
            cfg.fill_block_w = s.width;
            cfg.out.pic_w = s.width;
        }

        cfg.out.buffer_size = s.fullSize;

        cfg.out.block_offset_x = 0;
        cfg.out.block_offset_y = 0;

        cfg.fill_block_h = s.height;
        cfg.out.pic_h = s.height;

        cfg.fill_argb_color.a = 255;

        cfg.out.fill_cm = PPA_FILL_COLOR_MODE_RGB565;
        cfg.mode = PPA_TRANS_MODE_BLOCKING;

        esp_err_t err = ppa_do_fill(_ppaFill, &cfg);

        if (err == ESP_OK) return;

        Serial.printf("ppa_do_fill failed: %s\n", esp_err_to_name(err));
    }
#endif

    clsDef(rgb888to565(r, g, b));
}

void LCD_esp32::clsDef(uint16_t col){
    auto& s = _scr;
    if (s.usePal || s.bpp == _8BIT){
        memset(s.bLine8[0], (uint8_t)col, s.size);
    } else {
        uint16_t* scr = (uint16_t*)s.bLine16[0];

        if ((uint8_t)col == (uint8_t)(col >> 8)){
            memset(scr, col , s.fullSize);
        } else {
            int size = 0;
            uint16_t* cpy = scr;
            while (size++ < s.width) *scr++ = col;

            int dummy = 1; 
            int lines = s.maxY;  
            int copyBytes = s.lineSize;
            int offset = s.width;
        
            while (lines > 0){ 
                if (lines >= dummy){
                    memcpy(scr, cpy, copyBytes);
                    lines -= dummy;
                    scr += offset;
                    copyBytes <<= 1;
                    offset <<= 1;
                    dummy <<= 1;
                } else {
                    copyBytes =s.lineSize * lines;
                    memcpy(scr, cpy, copyBytes);
                    break;
                }
            }            
        }
    }
}

void LCD_esp32::cls(uint16_t col){
    auto& s = _scr;
    if (!s.inited) return;

#if IS_P4
    if (_ppaFill) {
        ppa_fill_oper_config_t cfg = {};

        if (s.usePal || s.bpp == _8BIT) {
            uint8_t c = (uint8_t)col;
            uint16_t color = ((uint16_t)c << 8) | c;   // 0xCCCC

            cfg.fill_argb_color.r = R16(color);
            cfg.fill_argb_color.g = G16(color);
            cfg.fill_argb_color.b = B16(color);

            cfg.out.buffer = (void*)s.bLine8[0];
            cfg.fill_block_w = s.width >> 1;
            cfg.out.pic_w = s.width >> 1;
        } else {
            cfg.fill_argb_color.r = R16(col);
            cfg.fill_argb_color.g = G16(col);
            cfg.fill_argb_color.b = B16(col);

            cfg.out.buffer = (void*)s.bLine16[0];
            cfg.fill_block_w = s.width;
            cfg.out.pic_w = s.width;
        }

        cfg.out.buffer_size = s.fullSize;
        cfg.out.block_offset_x = 0;
        cfg.out.block_offset_y = 0;

        cfg.fill_block_h = s.height;
        cfg.out.pic_h = s.height;

        cfg.fill_argb_color.a = 255;
        cfg.out.fill_cm = PPA_FILL_COLOR_MODE_RGB565;
        cfg.mode = PPA_TRANS_MODE_BLOCKING;

        esp_err_t err = ppa_do_fill(_ppaFill, &cfg);
        if (err == ESP_OK) return;

        Serial.printf("ppa_do_fill failed: %s\n", esp_err_to_name(err));
    }
#endif

    clsDef(col);
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
    if (_scr.dBuff && _sem_gui_ready && _sem_vsync_end){
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

void LCD_esp32::setPal(uint8_t index, uint8_t r, uint8_t g, uint8_t b){
    setPal(index, rgb888to565(r, g, b));
}

void LCD_esp32::scrToScr(){
    if (!_scr.inited || !_scr.dBuff) return;
    memcpy(_scr.fb0, _scr.fb1, _scr.fullSize);
}

void LCD_esp32::LUT(int &x, int &y, int xx, int yy, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    x = xx + (len * cosLUT[angle]) / 1000;
    y = yy + (len * sinLUT[angle]) / 1000;
}

int LCD_esp32::xLUT(int x, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    return x + (len * cosLUT[angle]) / 1000;
}

int LCD_esp32::yLUT(int y, int len, int angle) {
    angle %= LUT_SIZE;
    if (angle < 0) angle += 360;
    return y + (len * sinLUT[angle]) / 1000;
}

bool LCD_esp32::initBG(){
    auto& s = _scr;

    s.bg = (uint8_t*)allocateMemory(s.fullSize, true);
    if (!s.bg) return false;

    return true;
}

void LCD_esp32::scrToBg(){
    auto& s = _scr;
    if (!s.inited || !s.bg) return;
    memcpy(s.bg, ((s.usePal || s.bpp == _8BIT) ? (void*)s.bLine8[0] : (void*)s.bLine16[0]), s.fullSize);
}

void LCD_esp32::bgToScr(){
    auto& s = _scr;
    if (!s.inited || !s.bg) return;

    //esp_cache_msync(s.bg, s.fullSize, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    void* dst = (s.usePal || s.bpp == _8BIT) ? (void*)s.bLine8[0] : (void*)s.bLine16[0];

    asyncCopyRect(s.bg, dst, 0, 0, 0, 0, s.width, s.height);
}

bool LCD_esp32::asyncCopyRect(void* src, void* dst, 
                              int src_x, int src_y, 
                              int dst_x, int dst_y, 
                              int w, int h)
{
    if (_fbcpy == NULL) {
        if (!initFBCopy()) return false;
    }

    xSemaphoreTake(_fbCopySemaphore, 0);

    size_t bpp = (_scr.bpp == _16BIT && !_scr.usePal) ? 2 : 1;
    size_t data_size = (size_t)w * h * bpp;

    // Полный сброс кэша источника перед DMA
    esp_cache_msync(src, _scr.fullSize, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_INVALIDATE);

    esp_async_fbcpy_trans_desc_t trans = {
        .src_buffer         = src,
        .dst_buffer         = dst,
        .src_buffer_size_x  = _scr.width,
        .src_buffer_size_y  = _scr.height,
        .dst_buffer_size_x  = _scr.width,
        .dst_buffer_size_y  = _scr.height,

        .src_offset_x       = src_x,
        .src_offset_y       = src_y,
        .dst_offset_x       = dst_x,
        .dst_offset_y       = dst_y,

        .copy_size_x        = w,
        .copy_size_y        = h,
    };

    if (_scr.usePal || _scr.bpp == _8BIT) {
        trans.pixel_format_unique_id.color_type_id = COLOR_TYPE_ID(COLOR_SPACE_RAW, COLOR_PIXEL_RAW8);
    } else {
        trans.pixel_format_unique_id.color_type_id = COLOR_TYPE_ID(COLOR_SPACE_RGB, COLOR_PIXEL_RGB565);
    }

    esp_err_t err = esp_async_fbcpy(_fbcpy, &trans, fbcpy_done_callback, this);

    if (err != ESP_OK) {
        Serial.printf("fbcpy err: %d\n", err);
        return false;
    }

    if (xSemaphoreTake(_fbCopySemaphore, portMAX_DELAY)) {
        esp_cache_msync(dst, _scr.fullSize, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
        return true;
    } 

    Serial.println("FBCopy timeout");
    return false;
}
/*
bool LCD_esp32::asyncCopyRect(void* src, void* dst, 
                              int src_x, int src_y, 
                              int dst_x, int dst_y, 
                              int w, int h)
{
    if (_fbcpy == NULL) {
        if (!initFBCopy()) return false;
    }

    xSemaphoreTake(_fbCopySemaphore, 0);

    size_t bytes_per_pixel = (_scr.bpp == _16BIT && !_scr.usePal) ? 2 : 1;
    size_t data_size = (size_t)w * h * bytes_per_pixel;

    // Перед копированием
    esp_cache_msync(src, data_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

    esp_async_fbcpy_trans_desc_t trans = {
        .src_buffer         = src,
        .dst_buffer         = dst,
        .src_buffer_size_x  = _scr.width,
        .src_buffer_size_y  = _scr.height,
        .dst_buffer_size_x  = _scr.width,
        .dst_buffer_size_y  = _scr.height,

        .src_offset_x       = src_x,
        .src_offset_y       = src_y,
        .dst_offset_x       = dst_x,
        .dst_offset_y       = dst_y,

        .copy_size_x        = w,
        .copy_size_y        = h,
    };

    // Формат пикселей
    if (_scr.usePal || _scr.bpp == _8BIT) {
        trans.pixel_format_unique_id.color_type_id = COLOR_TYPE_ID(COLOR_SPACE_RAW, COLOR_PIXEL_RAW8);
    } else {
        trans.pixel_format_unique_id.color_type_id = COLOR_TYPE_ID(COLOR_SPACE_RGB, COLOR_PIXEL_RGB565);
    }

    esp_err_t err = esp_async_fbcpy(_fbcpy, &trans, fbcpy_done_callback, this);

    if (err != ESP_OK) {
        Serial.printf("esp_async_fbcpy failed: %d\n", err);
        return false;
    }

    if (xSemaphoreTake(_fbCopySemaphore, pdMS_TO_TICKS(2000)) == pdTRUE) {
        esp_cache_msync(dst, data_size, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
        return true;
    } else {
        Serial.println("FBCopy timeout!");
        return false;
    }
}
/*
bool LCD_esp32::asyncCopyRect(void* src, void* dst, 
                              int src_x, int src_y, 
                              int dst_x, int dst_y, 
                              int w, int h)
{
    if (_fbcpy == NULL) {
        if (!initFBCopy()) {
            Serial.println("FBCopy init failed");
            return false;
        }
    }

    xSemaphoreTake(_fbCopySemaphore, 0);

    size_t bytes_per_pixel = (_scr.bpp == _16BIT || !_scr.usePal) ? 2 : 1;
    size_t data_size = (size_t)w * h * bytes_per_pixel;

    // === 1. Перед копированием: записываем src в память ===
    esp_cache_msync(src, data_size, ESP_CACHE_MSYNC_FLAG_DIR_C2M);

    esp_async_fbcpy_trans_desc_t trans = {
        .src_buffer         = src,
        .dst_buffer         = dst,
        .src_buffer_size_x  = _scr.width,
        .src_buffer_size_y  = _scr.height,
        .dst_buffer_size_x  = _scr.width,
        .dst_buffer_size_y  = _scr.height,

        .src_offset_x       = src_x,
        .src_offset_y       = src_y,
        .dst_offset_x       = dst_x,
        .dst_offset_y       = dst_y,

        .copy_size_x        = w,
        .copy_size_y        = h,
    };

    // Правильная установка формата пикселей
    if (_scr.usePal || _scr.bpp == _8BIT) {
        trans.pixel_format_unique_id.color_type_id = 
            COLOR_TYPE_ID(COLOR_SPACE_RAW, COLOR_PIXEL_RAW8);
    } else {
        trans.pixel_format_unique_id.color_type_id = 
            COLOR_TYPE_ID(COLOR_SPACE_RGB, COLOR_PIXEL_RGB565);
    }

    esp_err_t err = esp_async_fbcpy(_fbcpy, &trans, fbcpy_done_callback, this);

    if (err != ESP_OK) {
        Serial.printf("esp_async_fbcpy failed: %d\n", err);
        return false;
    }

    // Ждём завершения DMA
    if (xSemaphoreTake(_fbCopySemaphore, pdMS_TO_TICKS(2000)) == pdTRUE) {
        // === 2. После DMA: инвалидируем кэш (только M2C без UNALIGNED) ===
        esp_cache_msync(dst, data_size, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
        return true;
    } else {
        Serial.println("FBCopy timeout!");
        return false;
    }
}
*/
#ifdef IS_P4
bool LCD_esp32::ppaFillInit(){
    if (_ppaFillInited) return true;

    ppa_client_config_t cfg = {};
    cfg.oper_type = PPA_OPERATION_FILL;
    cfg.max_pending_trans_num = 1; 
    err = ppa_register_client(&cfg, &_ppaFill);

    return (_ppaFillInited = (err == ESP_OK));
}

bool LCD_esp32::ppaCopyInit(){
    if (_ppaCopyInited) return true;

    ppa_client_config_t cfg = {};
    cfg.oper_type = PPA_OPERATION_SRM;
    cfg.max_pending_trans_num = 1;
    cfg.data_burst_length = PPA_DATA_BURST_LENGTH_64;
    err = ppa_register_client(&cfg, &_ppaCopy);

    return (_ppaCopyInited = (err == ESP_OK));
}

bool LCD_esp32::initFBCopy() 
{
    if (_fbcpy != NULL) return true;

    esp_async_fbcpy_config_t cfg = {};
    esp_err_t err = esp_async_fbcpy_install(&cfg, &_fbcpy);
    
    if (err != ESP_OK) {
        Serial.printf("esp_async_fbcpy_install failed: %d\n", err);
        return false;
    }

    _fbCopySemaphore = xSemaphoreCreateBinary();
    if (_fbCopySemaphore == NULL) {
        Serial.println("Failed to create semaphore");
        return false;
    }

    xSemaphoreTake(_fbCopySemaphore, 0);  // очистка

    Serial.println("Async FB Copy initialized OK");
    return true;
}

// ==================== STATIC CALLBACK ====================
bool LCD_esp32::fbcpy_done_callback(esp_async_fbcpy_handle_t mcp,
                                    esp_async_fbcpy_event_data_t *event_data,
                                    void *cb_args)
{
    LCD_esp32* lcd = static_cast<LCD_esp32*>(cb_args);
    BaseType_t higher_prio_task_woken = pdFALSE;

    if (lcd && lcd->_fbCopySemaphore) {
        xSemaphoreGiveFromISR(lcd->_fbCopySemaphore, &higher_prio_task_woken);
    }

    return (higher_prio_task_woken == pdTRUE);
}
#endif
