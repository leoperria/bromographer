#include "display.h"

#include <Arduino.h>
#include <string.h>
#include <Wire.h>

#include "config.h"

Display::Display() : oled_(U8G2_R0, U8X8_PIN_NONE) {
    memset(&last_, 0, sizeof(last_));
}

void Display::drawCellGlyph(const char c, const uint8_t col, const uint8_t rowTopPx) {
    const uint8_t x = static_cast<uint8_t>(col * kCellWidthPx);

    if (c >= 0 && c <= 4) {
        // Keep the old 5-step progress granularity: glyph 0..4 = 1..5 columns filled.
        const uint8_t fillPx = static_cast<uint8_t>(c + 1);
        oled_.drawBox(x, rowTopPx, fillPx, kCellHeightPx - 1);
        return;
    }

    if (c == 5) {
        // Selector arrow used by the ready screen.
        const uint8_t yMid = static_cast<uint8_t>(rowTopPx + (kCellHeightPx / 2));
        for (uint8_t i = 0; i < 5; ++i) {
            const uint8_t y0 = static_cast<uint8_t>(yMid - i);
            const uint8_t y1 = static_cast<uint8_t>(yMid + i);
            oled_.drawVLine(static_cast<uint8_t>(x + 2 + i), y0, static_cast<uint8_t>(y1 - y0 + 1));
        }
        return;
    }

    const char text[2] = {c, '\0'};
    const uint8_t baseline = static_cast<uint8_t>(rowTopPx + 11);
    oled_.drawStr(x, baseline, text);
}

void Display::drawTextLine(const char* line, const uint8_t rowTopPx) {
    for (uint8_t col = 0; col < Config::kDisplayCols; ++col) {
        drawCellGlyph(line[col], col, rowTopPx);
    }
}

void Display::begin() {
    Wire.begin();
    Wire.beginTransmission(Config::kDisplayI2cAddress);
    const uint8_t i2cStatus = Wire.endTransmission();
    (void)i2cStatus;

    oled_.setI2CAddress(static_cast<uint8_t>(Config::kDisplayI2cAddress << 1));
    oled_.begin();
    oled_.setFont(u8g2_font_8x13B_mf);
    oled_.setFontMode(1);
    oled_.setDrawColor(1);
    oled_.clearBuffer();
    oled_.sendBuffer();
}

void Display::render(const ScreenBuffer &screen) {
    bool changed = !hasLast_;
    for (uint8_t row = 0; row < Config::kDisplayRows; ++row) {
        const char *src = (row == 0) ? screen.line1 : screen.line2;
        char *old = (row == 0) ? last_.line1 : last_.line2;

        for (uint8_t col = 0; col < Config::kDisplayCols; ++col) {
            if (old[col] != src[col]) {
                changed = true;
                old[col] = src[col];
            }
        }
    }

    if (!changed) {
        return;
    }

    oled_.clearBuffer();
    drawTextLine(last_.line1, kLine1TopPx);
    drawTextLine(last_.line2, kLine2TopPx);
    oled_.sendBuffer();
    hasLast_ = true;
}
