#include "lovyan_st7789_driver.h"
#include "../../config/hardware_pins.h"
#include "../../config/hardware_config.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>

LovyanSt7789Driver::DisplayDevice::DisplayDevice() {
  {
    auto cfg = bus_.config();

    cfg.spi_host = SPI2_HOST;
    cfg.spi_mode = 0;
    cfg.freq_write = HardwareConfig::Display::SPI_WRITE_FREQ;
    cfg.freq_read = HardwareConfig::Display::SPI_READ_FREQ;
    cfg.spi_3wire = false;
    cfg.use_lock = true;
    cfg.dma_channel = SPI_DMA_CH_AUTO;

    cfg.pin_sclk = HardwarePins::Display::SCLK;
    cfg.pin_mosi = HardwarePins::Display::MOSI;
    cfg.pin_miso = HardwarePins::Display::MISO;
    cfg.pin_dc = HardwarePins::Display::DC;

    bus_.config(cfg);
    panel_.setBus(&bus_);
  }

  {
    auto cfg = panel_.config();

    cfg.pin_cs = HardwarePins::Display::CS;
    cfg.pin_rst = HardwarePins::Display::RST;
    cfg.pin_busy = -1;

    cfg.memory_width = HardwareConfig::Display::WIDTH;
    cfg.memory_height = HardwareConfig::Display::HEIGHT;
    cfg.panel_width = HardwareConfig::Display::WIDTH;
    cfg.panel_height = HardwareConfig::Display::HEIGHT;

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

  if (HardwarePins::Display::BL >= 0) {
    pinMode(HardwarePins::Display::BL, OUTPUT);
    digitalWrite(HardwarePins::Display::BL, HIGH);
  }

  lcd_.fillScreen(TFT_BLACK);
}

void LovyanSt7789Driver::clear() {
  lcd_.fillScreen(TFT_BLACK);
}

void LovyanSt7789Driver::clearRect(int x, int y, int w, int h) {
  if (w <= 0 || h <= 0) return;

  const int maxW = HardwareConfig::Display::WIDTH;
  const int maxH = HardwareConfig::Display::HEIGHT;

  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x >= maxW || y >= maxH) return;
  if (x + w > maxW) w = maxW - x;
  if (y + h > maxH) h = maxH - y;
  if (w <= 0 || h <= 0) return;

  lcd_.fillRect(x, y, w, h, TFT_BLACK);
}

void LovyanSt7789Driver::present() {
}

void LovyanSt7789Driver::drawEye(int x,
                                 int y,
                                 int radius,
                                 float openness,
                                 float tiltDeg,
                                 float squashY,
                                 float stretchX,
                                 float upperLid,
                                 float lowerLid) {
  openness = MathUtils::clamp(openness, 0.0f, 1.0f);
  squashY = MathUtils::clamp(squashY, 0.60f, 1.40f);
  stretchX = MathUtils::clamp(stretchX, 0.75f, 1.40f);
  upperLid = MathUtils::clamp(upperLid, 0.0f, 0.95f);
  lowerLid = MathUtils::clamp(lowerLid, 0.0f, 0.95f);
  tiltDeg = MathUtils::clamp(tiltDeg, -20.0f, 20.0f);

  const uint16_t kEyeMain = 0x7DFF;

  // Base openness keeps the eye alive; lids are applied as explicit top/bottom cuts.
  const float baseOpen = MathUtils::clamp(openness * (1.0f - (0.30f * (upperLid + lowerLid))), 0.10f, 1.0f);

  const int eyeWidth = static_cast<int>((radius * 2) * stretchX);
  int eyeHeight = static_cast<int>((radius * 2) * squashY * baseOpen);
  if (eyeHeight < 4) eyeHeight = 4;

  const int left = x - (eyeWidth / 2);
  const int top = y - (eyeHeight / 2);

  int corner = eyeWidth / 5;
  if (corner < 4) corner = 4;
  if (corner > eyeHeight / 2) corner = eyeHeight / 2;

  // EMO/Eilik-inspired rounded rectangle, no pupil.
  lcd_.fillRoundRect(left, top, eyeWidth, eyeHeight, corner, kEyeMain);

  // Top/bottom lid cuts create stronger expression readability.
  const int topCut = static_cast<int>(eyeHeight * upperLid * 0.70f);
  const int bottomCut = static_cast<int>(eyeHeight * lowerLid * 0.70f);
  if (topCut > 0) {
    lcd_.fillRect(left - 1, top - 1, eyeWidth + 2, topCut, TFT_BLACK);
  }
  if (bottomCut > 0) {
    lcd_.fillRect(left - 1, top + eyeHeight - bottomCut + 1, eyeWidth + 2, bottomCut, TFT_BLACK);
  }

  // Tilt as stronger shear to separate expressions (angry/sad/curious) visually.
  const int shear = static_cast<int>(tiltDeg * 0.18f);
  const int bandH = eyeHeight / 3;
  if (shear > 0) {
    lcd_.fillRect(left, top, shear, bandH, TFT_BLACK);
    lcd_.fillRect(left + eyeWidth - shear, top + eyeHeight - bandH, shear, bandH, TFT_BLACK);
  } else if (shear < 0) {
    const int s = -shear;
    lcd_.fillRect(left + eyeWidth - s, top, s, bandH, TFT_BLACK);
    lcd_.fillRect(left, top + eyeHeight - bandH, s, bandH, TFT_BLACK);
  }
}
void LovyanSt7789Driver::drawPupil(int x, int y, int radius) {
  (void)x;
  (void)y;
  (void)radius;
}

void LovyanSt7789Driver::drawText(int x, int y, const char* text) {
  lcd_.setTextColor(TFT_GREEN, TFT_BLACK);
  lcd_.setTextSize(2);
  lcd_.setCursor(x, y);
  lcd_.println(text);
}




