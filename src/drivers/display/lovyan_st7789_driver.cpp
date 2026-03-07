#include "lovyan_st7789_driver.h"
#include "../../config/hardware_pins.h"
#include "../../config/hardware_config.h"
#include "../../utils/math_utils.h"

LovyanSt7789Driver::DisplayDevice::DisplayDevice() {
  {
    auto cfg = bus_.config();

    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = HardwareConfig::Display::SPI_WRITE_FREQ;
    cfg.freq_read  = HardwareConfig::Display::SPI_READ_FREQ;
    cfg.spi_3wire = false;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;

    cfg.pin_sclk = HardwarePins::Display::SCLK;
    cfg.pin_mosi = HardwarePins::Display::MOSI;
    cfg.pin_miso = HardwarePins::Display::MISO;
    cfg.pin_dc   = HardwarePins::Display::DC;

    bus_.config(cfg);
    panel_.setBus(&bus_);
  }

  {
    auto cfg = panel_.config();

    cfg.pin_cs   = HardwarePins::Display::CS;
    cfg.pin_rst  = HardwarePins::Display::RST;
    cfg.pin_busy = -1;

    cfg.memory_width  = HardwareConfig::Display::WIDTH;
    cfg.memory_height = HardwareConfig::Display::HEIGHT;
    cfg.panel_width   = HardwareConfig::Display::WIDTH;
    cfg.panel_height  = HardwareConfig::Display::HEIGHT;

    cfg.offset_x = HardwareConfig::Display::OFFSET_X;
    cfg.offset_y = HardwareConfig::Display::OFFSET_Y;
    cfg.offset_rotation = 0;

    cfg.dummy_read_pixel = 8;
    cfg.dummy_read_bits = 1;
    cfg.readable = false;
    cfg.invert = HardwareConfig::Display::INVERT;
    cfg.rgb_order = HardwareConfig::Display::RGB_ORDER;
    cfg.dlen_16bit = false;
    cfg.bus_shared = false;

    panel_.config(cfg);
  }

  setPanel(&panel_);
}

LovyanSt7789Driver::LovyanSt7789Driver() = default;

void LovyanSt7789Driver::init() {
  lcd_.init();
  lcd_.setRotation(HardwareConfig::Display::ROTATION);
  lcd_.fillScreen(TFT_BLACK);
}

void LovyanSt7789Driver::clear() {
  lcd_.fillScreen(TFT_BLACK);
}

void LovyanSt7789Driver::present() {
}

void LovyanSt7789Driver::drawEye(int x, int y, int radius, float openness) {
  openness = MathUtils::clamp(openness, 0.0f, 1.0f);

  int eyeHeight = static_cast<int>((radius * 2) * openness);
  if (eyeHeight < 4) eyeHeight = 4;

  int topY = y - (eyeHeight / 2);

  lcd_.fillRoundRect(
    x - radius,
    topY,
    radius * 2,
    eyeHeight,
    eyeHeight / 2,
    TFT_WHITE
  );
}

void LovyanSt7789Driver::drawPupil(int x, int y, int radius) {
  lcd_.fillCircle(x, y, radius, TFT_BLACK);
}

void LovyanSt7789Driver::drawText(int x, int y, const char* text) {
  lcd_.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd_.setTextSize(2);
  lcd_.setCursor(x, y);
  lcd_.println(text);
}