#include "VGA_Driver.h"
#include "../LCD_esp32.h"

#ifdef LCD_DRIVER_VGA
#include <esp_LCD_panel_ops.h>

VGA_Driver::VGA_Driver(LCD_esp32 &lcd) : _lcd(lcd){
    
}

VGA_Driver::~VGA_Driver(){

}

bool VGA_Driver::selectMode(Screen_mode m){
    auto& s = _lcd._scr;

    switch (m){

        // =========================
        // 640x480 base
        // =========================
        case Mode640x480:
            _m = MODE640x480_60Hz;
            s.scale = 0;
            break;

        case Mode320x240:
            _m = MODE640x480_60Hz;
            s.scale = 1;
            break;

        case Mode160x120:
            _m = MODE640x480_60Hz;
            s.scale = 2;
            break;

        // =========================
        // unsupported modes
        // =========================
        default:
            Serial.printf(
                "[VGA] Unsupported mode: %d\n",
                (int)m
            );
            return false;
    }

    return true;
}

bool VGA_Driver::init(Screen_mode m, bool usePal, uint8_t bpp, bool dBuff) {
    auto& s = _lcd._scr;

    if (!selectMode(m)) return (s.inited = false);
    if (bpp != _8BIT && bpp != _16BIT) return (s.inited = false);

    Serial.println("\n[VGA] Starting...");

    s.usePal = usePal;

    // draw buffer format
    if (s.usePal) {
        s.bpp = _8BIT;      // framebuffer хранит индексы палитры
        s.shift = 0;
        s.dBuff = true;     // palette mode лучше всегда double buffer
    } else {
        s.bpp = bpp;
        s.shift = (bpp == _16BIT) ? 1 : 0;
        s.dBuff = dBuff;
    }

    _lcd.setScreenDimentions(_m.hRes, _m.vRes);

    // output format for RGB panel
    uint8_t panel_bpp = s.usePal ? _16BIT : s.bpp;

    if (!setRGBPanel(panel_bpp)) {
        Serial.println("ERROR: setRGBPanel failed");
        return (s.inited = false);
    }

    size_t req = 0;

    if (s.usePal) {
        int mul = scaleMul[s.scale];

        s.pal = (uint16_t*)malloc(256 * mul * sizeof(uint16_t));
        if (!s.pal) {
            Serial.println("ERROR: palette alloc failed");
            return (s.inited = false);
        }

        req = s.size; // 8-bit index buffer
        s.fb0 = (uint8_t*)_lcd.allocateMemory(req, true);
        s.fb1 = (uint8_t*)_lcd.allocateMemory(req, true);
    } else {
        req = s.fullSize;

        s.fb0 = (uint8_t*)_lcd.allocateMemory(req, true);

        if (s.dBuff) {
            s.fb1 = (uint8_t*)_lcd.allocateMemory(req, true);
        }
    }

    if (!s.fb0 || ((s.usePal || s.dBuff) && !s.fb1)) {
        Serial.println("ERROR: allocateMemory failed");
        return (s.inited = false);
    }

    Serial.println("Alloc...Ok");

    if (!_lcd.setBufferAddr()) {
        Serial.println("ERROR: setBufferAddr failed");
        return (s.inited = false);
    }

    if (s.usePal || s.dBuff) {
        if (!_lcd.regSemaphore()) return (s.inited = false);
    } else {
        _lcd._sem_vsync_end = nullptr;
        _lcd._sem_gui_ready = nullptr;
    }

    regCallBack();

    if (!initPanel()) {
        Serial.println("ERROR: initPanel failed");
        return (s.inited = false);
    }

    Serial.println("VGA init...done\n");
    return (s.inited = true);
}

int VGA_Driver::optimal_bounce_buffer_px(){
    auto& s = _lcd._scr;

    int res = 0;
    if (_m.hRes == 640 && _m.vRes == 480) {
        res = 30720 >> s.shift;   // 16bit = 15360 px, 8bit = 30720 px
    }

    _lastPos = _m.hRes * _m.vRes - res;
    _lines = (res / _m.hRes) >> s.scale;
    _pixels = _m.hRes >> 3;
    _copyBytes = _m.hRes << s.shift; 
    _copyBytes2x = _copyBytes << 1;
    _skip = (_m.hRes >> 2) << s.shift;

    return (_bounceBufferSize_px = res);
}

bool VGA_Driver::setRGBPanel(uint8_t panel_bpp){
    auto& m = _m;
    auto& s = _lcd._scr;
    auto& p = _lcd._pins;

    panel_config = {};
    _lcd.getAlignSize();

    //Timing
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_config.timings.pclk_hz                = m.pclk_hz;
    panel_config.timings.h_res                  = m.hRes;
    panel_config.timings.v_res                  = m.vRes;

    panel_config.timings.hsync_pulse_width      = m.hSync;
    panel_config.timings.hsync_back_porch       = m.hBack;
    panel_config.timings.hsync_front_porch      = m.hFront;
    panel_config.timings.flags.hsync_idle_low   = (m.hPol == 1) ^ 1;

    panel_config.timings.vsync_pulse_width      = m.vSync;
    panel_config.timings.vsync_back_porch       = m.vBack;
    panel_config.timings.vsync_front_porch      = m.vFront;
    panel_config.timings.flags.vsync_idle_low   = (m.vPol == 1) ^ 1;
    
    panel_config.timings.flags.de_idle_high     = true;
    panel_config.timings.flags.pclk_active_neg  = true;
    panel_config.timings.flags.pclk_idle_high   = true;

    //Panel config
    panel_config.data_width             = panel_bpp;        
    panel_config.bits_per_pixel         = panel_bpp;   
    panel_config.num_fbs                = 0;
    panel_config.bounce_buffer_size_px  = optimal_bounce_buffer_px();
    panel_config.sram_trans_align       = _lcd._sramAlign;
    panel_config.psram_trans_align      = _lcd._psramAlign;
    Serial.printf("Align sram: %d, psram: %d\n", _lcd._sramAlign, _lcd._psramAlign);
    //panel_config.dma_burst_size = 64;

    //Pins config
    if (panel_bpp == _16BIT){
        // B0..B4
        for (int i = 0; i < 5; i++)
            panel_config.data_gpio_nums[i] = p.b[i];

        // G0..G5
        for (int i = 0; i < 6; i++)
            panel_config.data_gpio_nums[5 + i] = p.g[i];

        // R0..R4
        for (int i = 0; i < 5; i++)
            panel_config.data_gpio_nums[11 + i] = p.r[i];
    } else {
        panel_config.data_gpio_nums[0] = p.b[3];
        panel_config.data_gpio_nums[1] = p.b[4];
        panel_config.data_gpio_nums[2] = p.g[3];
        panel_config.data_gpio_nums[3] = p.g[4];
        panel_config.data_gpio_nums[4] = p.g[5];
        panel_config.data_gpio_nums[5] = p.r[2];
        panel_config.data_gpio_nums[6] = p.r[3];
        panel_config.data_gpio_nums[7] = p.r[4];
    }
    panel_config.hsync_gpio_num = p.h;
    panel_config.vsync_gpio_num = p.v;
    panel_config.de_gpio_num = -1;
    panel_config.pclk_gpio_num = p.pClk;
    panel_config.disp_gpio_num = -1;

    //Flags
    panel_config.flags.disp_active_low = true;
    panel_config.flags.refresh_on_demand = false;
    panel_config.flags.fb_in_psram = false;
    panel_config.flags.double_fb = false;
    panel_config.flags.no_fb = true;
    panel_config.flags.bb_invalidate_cache = false;
    
    esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
    if (err != ESP_OK) {
        Serial.printf("esp_lcd_new_rgb_panel error: %d\n", err);
        return false;
    }

    Serial.println("RGB Panel set...Ok");
    return true;
}

void VGA_Driver::regCallBack(){
    esp_lcd_rgb_panel_event_callbacks_t cb = {
        .on_color_trans_done = nullptr,
        .on_vsync            = on_vsync,
        .on_bounce_empty = (_lcd._scr.usePal ? on_bounce_empty_p4_pal : on_bounce_empty_p4), /*(
            _usePal
                ? on_bounce_empty_pal
                : (IS_P4 ? on_bounce_empty_p4 : on_bounce_empty)
        ), */             
        .on_frame_buf_complete = nullptr
    };

    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cb, this)); 
    Serial.println("Registered callBacks...Ok"); 
}
   
bool VGA_Driver::initPanel(){
    err = esp_lcd_panel_reset(panel_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to reset panel: %s\n", esp_err_to_name(err));
        return false;
    }

    err = esp_lcd_panel_init(panel_handle);
    if (err != ESP_OK) {
        Serial.printf("ERROR: Failed to init panel: %s\n", esp_err_to_name(err));
        return false;
    }

    Serial.println(_bounceBufferSize_px);
    Serial.println("Init panel complete...Ok");
    return true;
}

bool IRAM_ATTR VGA_Driver::on_vsync(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx){
    VGA_Driver* vga = (VGA_Driver*)user_ctx;

    vga->_lcd._timer++;
    //auto& s = vga->_lcd._scr;
    return false;
}

bool IRAM_ATTR VGA_Driver::on_bounce_empty_p4(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx){
    VGA_Driver* vga = (VGA_Driver*) user_ctx;

    auto& s = vga->_lcd._scr;
    int shift = s.shift;
    int bytes = 8 << shift;
    int fix   = len_bytes - bytes;
    int position = pos_px << shift;
    int fill = position + len_bytes + bytes;    

    if (s.scale == 0){
        uint8_t *dest = (uint8_t*)bounce_buf;
        uint8_t *sour = (uint8_t*)s.fb0 + (pos_px << s.shift);

        if (fill <= s.fullSize) memcpy(dest + fix, sour + len_bytes, bytes);        
            memcpy(dest, sour + bytes, fix);    
    } else {
        uint32_t* dest = (uint32_t*)((uint8_t*)bounce_buf + (vga->_m.hRes << shift) - bytes);

        uint32_t col;        
        int lines           = vga->_lines - 1;
        int pixels          = vga->_pixels;
        int copyBytes       = vga->_copyBytes;
        int copyBytes2x     = vga->_copyBytes2x;
        int skip            = vga->_skip;
        uint32_t* savePos   = nullptr; 

        if (s.scale == 1){
            if (s.bpp == _16BIT){
                uint16_t* sour = (uint16_t*)s.fb0 + (pos_px >> 2);
                uint16_t* secondSour = sour + 4;
                uint16_t* firstSour = sour + (len_bytes >> 3);

                for (int i = 0; i < pixels; i++){
                    col = *sour++; *dest++ = col | (col << 16);
                    col = *sour++; *dest++ = col | (col << 16);
                    col = *sour++; *dest++ = col | (col << 16);
                    col = *sour++; *dest++ = col | (col << 16);
                }  

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                        col = *sour++; *dest++ = col | (col << 16);
                    }    
                    memcpy(dest, savePos, copyBytes); dest += skip;
                } 

                col = *firstSour++; *dest++ = col | (col << 16);
                col = *firstSour++; *dest++ = col | (col << 16);
                col = *firstSour++; *dest++ = col | (col << 16);
                col = *firstSour++; *dest++ = col | (col << 16);

                pixels--;
                dest = (uint32_t*)(bounce_buf);
                while (pixels-- > 0){
                    col = *secondSour++; *dest++ = col | (col << 16);
                    col = *secondSour++; *dest++ = col | (col << 16);
                    col = *secondSour++; *dest++ = col | (col << 16);
                    col = *secondSour++; *dest++ = col | (col << 16);
                }                 
            } else {
                uint8_t* sour = (uint8_t*)s.fb0 + (pos_px >> 2);
                uint8_t* secondSour = sour + 4;
                uint8_t* firstSour = sour + (len_bytes >> 2);

                for (int i = 0; i < pixels; i++){
                    col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                    col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                }  
                
                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                        col = *sour++; col |= (*sour++ << 16); col |= col << 8; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);
                    dest += skip;
                }    
                
                col = *firstSour++; col |= (*firstSour++ << 16); col |= col << 8; *dest++ = col;
                col = *firstSour++; col |= (*firstSour++ << 16); col |= col << 8; *dest++ = col;

                pixels--;
                dest = (uint32_t*)(bounce_buf);
                while (pixels-- > 0){
                    col = *secondSour++; col |= (*secondSour++ << 16); col |= col << 8; *dest++ = col;
                    col = *secondSour++; col |= (*secondSour++ << 16); col |= col << 8; *dest++ = col;
                }                 
            }
        } else {
            int skip2 = skip << 1;

            if (s.bpp == _16BIT){
                uint16_t* sour = (uint16_t*)s.fb0 + (pos_px >> 4);
                uint16_t* secondSour = sour + 2;
                uint16_t* firstSour = sour + (len_bytes >> 3);

                savePos = dest;
                for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                }  
                memcpy(dest, savePos, copyBytes); dest += skip;
                memcpy(dest, savePos, copyBytes); dest += skip; 

                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                        col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);   dest += skip;
                    memcpy(dest, savePos, copyBytes2x); dest += skip2;
                }

                col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                col = *sour++; col |= (col << 16); *dest++ = col; *dest++ = col;                

                pixels--;
                dest = (uint32_t*)(bounce_buf);
                while (pixels-- > 0){
                        col = *secondSour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                        col = *secondSour++; col |= (col << 16); *dest++ = col; *dest++ = col;
                }                 
            } else {
                uint8_t* sour = (uint8_t*)s.fb0 + (pos_px >> 4);
                uint8_t* secondSour = sour + 2;
                uint8_t* firstSour = sour + (len_bytes >> 4); 

                savePos = dest;
                for (int i = 0; i < pixels; i++){
                    col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                    col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                }  
                memcpy(dest, savePos, copyBytes); dest += skip;
                memcpy(dest, savePos, copyBytes); dest += skip;                 
                
                while (lines-- > 0){
                    savePos = dest;
                    for (int i = 0; i < pixels; i++){
                        col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                        col = *sour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                    }    
                    memcpy(dest, savePos, copyBytes);   dest += skip;
                    memcpy(dest, savePos, copyBytes2x); dest += skip2;
                } 
                
                col = *firstSour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                col = *firstSour++; col |= (col << 16); col |= col << 8; *dest++ = col;  
                
                pixels--;
                dest = (uint32_t*)(bounce_buf);
                while (pixels-- > 0){
                    col = *secondSour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                    col = *secondSour++; col |= (col << 16); col |= col << 8; *dest++ = col;
                }                    
            }
        }
    }

    BaseType_t hp_task_woken = pdFALSE;    
    if (s.dBuff && pos_px >= vga->_lastPos){
        if (xSemaphoreTakeFromISR(vga->_lcd._sem_gui_ready, &hp_task_woken) == pdTRUE){
            std::swap(s.fb0, s.fb1);
            if (s.bpp == _16BIT) std::swap(s.fLine16, s.bLine16);        
            if (s.usePal || s.bpp == _8BIT) std::swap(s.fLine8, s.bLine8);        
            xSemaphoreGiveFromISR(vga->_lcd._sem_vsync_end, &hp_task_woken);
        }
    }

    return (hp_task_woken == pdTRUE);
}

bool IRAM_ATTR VGA_Driver::on_bounce_empty_p4_pal(esp_lcd_panel_handle_t panel, void *bounce_buf, int pos_px, int len_bytes, void *user_ctx){
    VGA_Driver* vga = (VGA_Driver*) user_ctx;

    auto& s = vga->_lcd._scr;

    // P4 shift fix: 8 output pixels
    int bytes = 8 << 1;              // 8 pixels * RGB565 2 bytes
    int fix   = len_bytes - bytes;

    int outPixels = len_bytes >> 1;  // сколько RGB565 пикселей надо выдать
    int srcPos    = pos_px + 8;      // +8 пикселей, не байт

    uint32_t* dest  = (uint32_t*)bounce_buf;
    if (s.scale == 0) {
        uint32_t c0, c1;
        int loops = outPixels >> 3;  // 8 pixels за цикл

        uint16_t* pal16 = s.pal;
        uint8_t*  sour  = (uint8_t*)s.fb0 + srcPos;        

        while (loops--) {
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = c0 | (c1 << 16);
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = c0 | (c1 << 16);
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = c0 | (c1 << 16);
            c0 = pal16[*sour++]; c1 = pal16[*sour++]; *dest++ = c0 | (c1 << 16);
        }

        // хвост, если дошли до конца framebuffer
        if ((srcPos + outPixels) > s.size) {
            sour = (uint8_t*)s.fb0;
        }
    } else if (s.scale == 1){
        uint32_t* pal32 = (uint32_t*)s.pal;
        uint32_t* d32   = (uint32_t*)bounce_buf;

        int outW = s.width << 1;

        int p = pos_px + 8;
        int left = outPixels;

        // вместо % и /
        int outY = p / outW;
        int outX = p - outY * outW;

        while (left > 0) {
            int srcY = outY >> 1;
            int srcX = outX >> 1;

            int n = outW - outX;
            if (n > left) n = left;

            uint8_t* sour = (uint8_t*)s.fb0 + srcY * s.width + srcX;

            int pairs = n >> 1;

            while (pairs >= 4) {
                *d32++ = pal32[*sour++];
                *d32++ = pal32[*sour++];
                *d32++ = pal32[*sour++];
                *d32++ = pal32[*sour++];
                pairs -= 4;
            }

            while (pairs--) {
                *d32++ = pal32[*sour++];
            }

            if (n & 1) {
                uint16_t* d16 = (uint16_t*)d32;
                *d16++ = ((uint16_t*)s.pal)[*sour];
                d32 = (uint32_t*)d16;
            }

            left -= n;
            outY++;
            outX = 0;
        }        
    } else {
        uint64_t* pal64 = (uint64_t*)s.pal;
        uint64_t* d64   = (uint64_t*)bounce_buf;

        int outW = s.width << 2;   // 160 -> 640, 320 -> 1280

        int p = pos_px + 8;
        int left = outPixels;

        int outY = p / outW;
        int outX = p - outY * outW;

        while (left > 0) {
            int srcY = outY >> 2;
            int srcX = outX >> 2;

            int n = outW - outX;
            if (n > left) n = left;

            uint8_t* sour = (uint8_t*)s.fb0 + srcY * s.width + srcX;

            int quads = n >> 2;

            while (quads >= 4) {
                *d64++ = pal64[*sour++];
                *d64++ = pal64[*sour++];
                *d64++ = pal64[*sour++];
                *d64++ = pal64[*sour++];
                quads -= 4;
            }

            while (quads--) {
                *d64++ = pal64[*sour++];
            }

            int tail = n & 3;
            if (tail) {
                uint16_t* d16 = (uint16_t*)d64;
                uint16_t c = ((uint16_t*)s.pal)[*sour];

                while (tail--) {
                    *d16++ = c;
                }

                d64 = (uint64_t*)d16;
            }

            left -= n;
            outY++;
            outX = 0;
        }
    }

    BaseType_t hp_task_woken = pdFALSE;    
    if (s.dBuff && pos_px >= vga->_lastPos){
        if (xSemaphoreTakeFromISR(vga->_lcd._sem_gui_ready, &hp_task_woken) == pdTRUE){
            std::swap(s.fb0, s.fb1);
            if (s.bpp == _16BIT) std::swap(s.fLine16, s.bLine16);        
            if (s.usePal || s.bpp == _8BIT) std::swap(s.fLine8, s.bLine8);        
            xSemaphoreGiveFromISR(vga->_lcd._sem_vsync_end, &hp_task_woken);
        }
    }

    return (hp_task_woken == pdTRUE);
}

#endif

/*
    uint16_t* pal = s.pal;
    uint8_t* src  = (uint8_t*)s.fb1;      // front index buffer
    uint32_t* dst = (uint32_t*)bounce_buf;

    int pixels = len_bytes >> 1;

    if (s.scale == 0) {
        int p = pos_px + 8;               // P4 shift fix

        for (int i = 0; i < pixels; i += 2) {
            if (p >= s.size) p -= s.size;
            uint32_t c0 = pal[src[p++]];

            if (p >= s.size) p -= s.size;
            uint32_t c1 = pal[src[p++]];

            *dst++ = c0 | (c1 << 16);
        }
    } else {
        // пока просто чёрный, чтобы не мусорить
        memset(bounce_buf, 0, len_bytes);
    }
*/