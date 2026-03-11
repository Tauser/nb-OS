#include "lovyan_st7789_driver.h"
#include "../../config/hardware_pins.h"
#include "../../config/hardware_config.h"
#include "../../utils/math_utils.h"
#include <Arduino.h>
#include <math.h>

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

namespace {
constexpr uint16_t kEyeMain = 0x9E7F;

float degToRad(float deg) {
  return deg * 0.01745329251f;
}

float lerpF(float a, float b, float t) {
  return a + ((b - a) * t);
}

void fillTrapezoidRows(lgfx::LGFX_Device& lcd,
                       int centerX,
                       int top,
                       int height,
                       float topWidth,
                       float bottomWidth,
                       float leftCantDeg,
                       float rightCantDeg,
                       uint16_t color) {
  if (height <= 0 || topWidth <= 1.0f || bottomWidth <= 1.0f) {
    return;
  }

  const float leftSlope = tanf(degToRad(leftCantDeg));
  const float rightSlope = tanf(degToRad(rightCantDeg));
  const float halfH = static_cast<float>(height - 1) * 0.5f;

  for (int row = 0; row < height; ++row) {
    const float t = (height <= 1) ? 0.0f : (static_cast<float>(row) / static_cast<float>(height - 1));
    const float width = lerpF(topWidth, bottomWidth, t);
    const float yBias = static_cast<float>(row) - halfH;

    const float leftXf = static_cast<float>(centerX) - (width * 0.5f) + (leftSlope * yBias);
    const float rightXf = static_cast<float>(centerX) + (width * 0.5f) + (rightSlope * yBias);

    const int leftX = static_cast<int>(leftXf);
    const int rightX = static_cast<int>(rightXf);
    const int w = rightX - leftX + 1;
    if (w > 0) {
      lcd.fillRect(leftX, top + row, w, 1, color);
    }
  }
}

void applyCornerCuts(lgfx::LGFX_Device& lcd,
                     int left,
                     int top,
                     int width,
                     int height,
                     const ResolvedEyeShape& shape) {
  const int minDim = (width < height) ? width : height;
  if (minDim < 6) {
    return;
  }

  const int tl = static_cast<int>((1.0f - shape.roundTopLeft) * minDim * 0.30f);
  const int tr = static_cast<int>((1.0f - shape.roundTopRight) * minDim * 0.30f);
  const int bl = static_cast<int>((1.0f - shape.roundBottomLeft) * minDim * 0.30f);
  const int br = static_cast<int>((1.0f - shape.roundBottomRight) * minDim * 0.30f);

  if (tl > 0) {
    lcd.fillTriangle(left, top, left + tl, top, left, top + tl, TFT_BLACK);
  }
  if (tr > 0) {
    lcd.fillTriangle(left + width - 1, top, left + width - 1 - tr, top, left + width - 1, top + tr, TFT_BLACK);
  }
  if (bl > 0) {
    lcd.fillTriangle(left, top + height - 1, left + bl, top + height - 1, left, top + height - 1 - bl, TFT_BLACK);
  }
  if (br > 0) {
    lcd.fillTriangle(left + width - 1, top + height - 1, left + width - 1 - br, top + height - 1, left + width - 1, top + height - 1 - br, TFT_BLACK);
  }
}

void applyLidCuts(lgfx::LGFX_Device& lcd,
                  int left,
                  int top,
                  int width,
                  int height,
                  float upperLid,
                  float lowerLid) {
  const int topCut = static_cast<int>(height * upperLid * 0.58f);
  const int bottomCut = static_cast<int>(height * lowerLid * 0.58f);
  if (topCut > 0) {
    lcd.fillRect(left - 1, top - 1, width + 2, topCut, TFT_BLACK);
  }
  if (bottomCut > 0) {
    lcd.fillRect(left - 1, top + height - bottomCut + 1, width + 2, bottomCut, TFT_BLACK);
  }
}

void applyTiltShear(lgfx::LGFX_Device& lcd,
                    int left,
                    int top,
                    int width,
                    int height,
                    float tiltDeg) {
  const int shear = static_cast<int>(tiltDeg * 0.16f);
  const int bandH = height / 3;
  if (shear > 0) {
    lcd.fillRect(left, top, shear, bandH, TFT_BLACK);
    lcd.fillRect(left + width - shear, top + height - bandH, shear, bandH, TFT_BLACK);
  } else if (shear < 0) {
    const int s = -shear;
    lcd.fillRect(left + width - s, top, s, bandH, TFT_BLACK);
    lcd.fillRect(left, top + height - bandH, s, bandH, TFT_BLACK);
  }
}
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

void LovyanSt7789Driver::drawLegacyEye(int x,
                                       int y,
                                       int radius,
                                       float openness,
                                       float tiltDeg,
                                       float squashY,
                                       float stretchX,
                                       float roundness,
                                       float upperLid,
                                       float lowerLid) {
  openness = MathUtils::clamp(openness, 0.0f, 1.0f);
  squashY = MathUtils::clamp(squashY, 0.60f, 1.40f);
  stretchX = MathUtils::clamp(stretchX, 0.75f, 1.40f);
  upperLid = MathUtils::clamp(upperLid, 0.0f, 0.95f);
  lowerLid = MathUtils::clamp(lowerLid, 0.0f, 0.95f);
  roundness = MathUtils::clamp(roundness, 0.20f, 0.90f);
  tiltDeg = MathUtils::clamp(tiltDeg, -20.0f, 20.0f);

  const float baseOpen = MathUtils::clamp(openness * (1.0f - (0.30f * (upperLid + lowerLid))), 0.10f, 1.0f);

  const int eyeWidth = static_cast<int>((radius * 2) * stretchX);
  int eyeHeight = static_cast<int>((radius * 2) * squashY * baseOpen);
  if (eyeHeight < 4) eyeHeight = 4;

  const int left = x - (eyeWidth / 2);
  const int top = y - (eyeHeight / 2);

  int corner = static_cast<int>((eyeHeight * roundness) + 1.5f);
  if (corner < 3) corner = 3;
  if (corner > eyeHeight / 2) corner = eyeHeight / 2;

  lcd_.fillRoundRect(left, top, eyeWidth, eyeHeight, corner, kEyeMain);
  applyLidCuts(lcd_, left, top, eyeWidth, eyeHeight, upperLid, lowerLid);
  applyTiltShear(lcd_, left, top, eyeWidth, eyeHeight, tiltDeg);
}

void LovyanSt7789Driver::drawEye(int x,
                                 int y,
                                 int radius,
                                 float openness,
                                 float tiltDeg,
                                 float squashY,
                                 float stretchX,
                                 float roundness,
                                 float upperLid,
                                 float lowerLid) {
  drawLegacyEye(x, y, radius, openness, tiltDeg, squashY, stretchX, roundness, upperLid, lowerLid);
}

void LovyanSt7789Driver::drawEyeShape(int x,
                                      int y,
                                      int radius,
                                      float openness,
                                      float tiltDeg,
                                      float squashY,
                                      float stretchX,
                                      float upperLid,
                                      float lowerLid,
                                      const ResolvedEyeShape& shapeIn) {
  ResolvedEyeShape shape = shapeIn;
  shape.sanitize();

  openness = MathUtils::clamp(openness, 0.0f, 1.0f);
  squashY = MathUtils::clamp(squashY, 0.60f, 1.40f);
  stretchX = MathUtils::clamp(stretchX, 0.75f, 1.40f);
  upperLid = MathUtils::clamp(upperLid, 0.0f, 0.95f);
  lowerLid = MathUtils::clamp(lowerLid, 0.0f, 0.95f);
  tiltDeg = MathUtils::clamp(tiltDeg, -20.0f, 20.0f);

  const float baseOpen = MathUtils::clamp(openness * (1.0f - (0.30f * (upperLid + lowerLid))), 0.10f, 1.0f);

  const int baseEyeWidth = static_cast<int>((radius * 2) * stretchX);
  int eyeHeight = static_cast<int>((radius * 2) * squashY * baseOpen);
  if (eyeHeight < 4) eyeHeight = 4;

  const int top = y - (eyeHeight / 2);

  const float topWidthF = static_cast<float>(baseEyeWidth) * shape.topWidthScale;
  const float bottomWidthF = static_cast<float>(baseEyeWidth) * shape.bottomWidthScale;
  const int maxWidth = static_cast<int>((topWidthF > bottomWidthF) ? topWidthF : bottomWidthF);
  const int bodyLeft = x - (maxWidth / 2);

  if (shape.mode == EyeShapeMode::RoundedRect) {
    drawLegacyEye(x,
                  y,
                  radius,
                  openness,
                  tiltDeg,
                  squashY,
                  stretchX,
                  faceShapeFallbackRoundness(shape),
                  upperLid,
                  lowerLid);
    return;
  }

  if (shape.mode == EyeShapeMode::SoftCapsule) {
    const int bodyWidth = static_cast<int>(lerpF(topWidthF, bottomWidthF, 0.5f));
    const int left = x - (bodyWidth / 2);
    int cap = eyeHeight / 2;
    if (cap < 2) cap = 2;
    lcd_.fillRoundRect(left, top, bodyWidth, eyeHeight, cap, kEyeMain);
    applyCornerCuts(lcd_, left, top, bodyWidth, eyeHeight, shape);
    applyLidCuts(lcd_, left, top, bodyWidth, eyeHeight, upperLid, lowerLid);
    applyTiltShear(lcd_, left, top, bodyWidth, eyeHeight, tiltDeg);
    return;
  }

  // Trapezoid and wedge share scanline fill; wedge semantics come from profile
  // parameters (stronger asymmetry and cant).
  fillTrapezoidRows(lcd_,
                    x,
                    top,
                    eyeHeight,
                    topWidthF,
                    bottomWidthF,
                    shape.leftCantDeg,
                    shape.rightCantDeg,
                    kEyeMain);

  applyCornerCuts(lcd_, bodyLeft, top, maxWidth, eyeHeight, shape);
  applyLidCuts(lcd_, bodyLeft, top, maxWidth, eyeHeight, upperLid, lowerLid);
  applyTiltShear(lcd_, bodyLeft, top, maxWidth, eyeHeight, tiltDeg);
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

