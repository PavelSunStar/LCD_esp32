#include "LCD_Driver.h"
#include "../LCD_esp32.h"

#ifdef LCD_DRIVER_LCD
#include <esp_LCD_panel_ops.h>
#include "esp_cache.h"

// весь код MIPI драйвера тут
LCD_Driver::LCD_Driver(LCD_esp32 &lcd) : _lcd(lcd){
    
}

LCD_Driver::~LCD_Driver(){

}

bool LCD_Driver::selectMode(Screen_mode m){
    auto& s = _lcd._scr;

    switch (m){

        // =========================
        // 640x480 base
        // =========================
        case Mode640x480:
            _m = MODE640x480_60Hz;
            s.scale = 0;
            break;
/*
        case Mode320x240:
            _m = MODE640x480_60Hz;
            s.scale = 1;
            break;

        case Mode160x120:
            _m = MODE640x480_60Hz;
            s.scale = 2;
            break;
*/
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

bool LCD_Driver::init(Screen_mode m, bool usePal, uint8_t bpp, bool dBuff){
    auto& s = _lcd._scr;

    if (!selectMode(m)) return (s.inited = false);
    if (bpp != _8BIT && bpp != _16BIT) return (s.inited = false);

    s.bpp = bpp;
    s.dBuff = dBuff;
    Serial.println("\n[LCD] Starting...");
    _lcd.setScreenDimentions(_m.hRes, _m.vRes);

    if (!setRGBPanel()) {
        Serial.println("ERROR: setRGBPanel failed");
        return (s.inited = false);
    }  
 
    if (!initPanel()){
        Serial.println("ERROR: initPanel failed");
        return (s.inited = false);
    } 

    esp_lcd_rgb_panel_get_frame_buffer(panel_handle, s.dBuff ? 2 : 1, &s.fb0, &s.fb1);
    memset(s.fb0, 0, s.fullSize);
    memmove(s.fb0, (uint8_t*)s.fb0 + (8 << s.shift), s.fullSize - (8 << s.shift));

    if (!_lcd.setBufferAddr()){
        Serial.println("ERROR: setBufferAddr failed");
        return (s.inited = false);
    }

    return (s.inited = true);
}

bool LCD_Driver::setRGBPanel(){
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
    panel_config.data_width             = s.bpp;        
    panel_config.bits_per_pixel         = s.bpp;   
    panel_config.num_fbs                = (s.dBuff ? 2 : 1);
    panel_config.bounce_buffer_size_px  = 0; //optimal_bounce_buffer_px();
    panel_config.sram_trans_align       = _lcd._sramAlign;
    panel_config.psram_trans_align      = _lcd._psramAlign;
    //panel_config.dma_burst_size = 64;

    //Pins config
    if (s.bpp == _16BIT/* || _usePal*/){
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
    panel_config.flags.fb_in_psram = true;
    panel_config.flags.double_fb = (s.dBuff ? true : false);
    panel_config.flags.no_fb = false;
    panel_config.flags.bb_invalidate_cache = false;
    
    esp_err_t err = esp_lcd_new_rgb_panel(&panel_config, &panel_handle);
    if (err != ESP_OK) {
        Serial.printf("esp_lcd_new_rgb_panel error: %d\n", err);
        return false;
    }

    Serial.println("RGB Panel set...Ok");
    return true;
}

bool LCD_Driver::initPanel(){
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

    err = esp_lcd_panel_disp_on_off(panel_handle, true);

    Serial.println("Init panel complete...Ok");
    return true;
}

void LCD_Driver::flush(int shiftPix){
    auto& s = _lcd._scr;

    if (_first){
        _first = false;
        shiftPix = 8;

        memcpy(
            s.fb0,
            (uint8_t*)s.fb0 + (8 << s.shift),
            s.fullSize - (8 << s.shift)
        );   
    }

    esp_cache_msync(
        s.fb0,
        s.fullSize,
        ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED
    );

    esp_lcd_panel_draw_bitmap(
        panel_handle,
        0, 0,
        s.width,
        s.height,
        s.fb0
    );
}

#endif

/*
esp_async_fbcpy_trans_desc_t t = {};

t.src_buffer = s.fb0;
t.dst_buffer = s.fb0;

t.src_buffer_size_x = s.width;
t.src_buffer_size_y = s.height;
t.dst_buffer_size_x = s.width;
t.dst_buffer_size_y = s.height;

t.src_offset_x = 8;
t.src_offset_y = 0;

t.dst_offset_x = 0;
t.dst_offset_y = 0;

t.copy_size_x = s.width - 8;
t.copy_size_y = s.height;

t.pixel_format_unique_id = COLOR_PIXEL_RGB565;

esp_async_fbcpy(_fbcpy, &t, nullptr, nullptr);
*/