#include "display.h"

#include <Arduino.h>
#include <string.h>
#include <Wire.h>

#include "config.h"

Display::Display() : lcd_(Config::kLcdI2cAddress, Config::kLcdCols, Config::kLcdRows) {
    memset(&last_, 0, sizeof(last_));
}

void Display::defineCustomChars() {
    // 1..5 column bar glyphs (slots 0..4)
    uint8_t bar1[8] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10};
    uint8_t bar2[8] = {0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18};
    uint8_t bar3[8] = {0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C};
    uint8_t bar4[8] = {0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E};
    uint8_t bar5[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F};

    // Selector arrow (slot 5): scaled-down chunky left-pointing triangle.
    // Keep one empty row at top/bottom to avoid clipped pixels on some LCDs.
    uint8_t arrow[8] = {0x08, 0x0C, 0x0E, 0x0F, 0x0E, 0x0C, 0x08, 0x00};

    lcd_.createChar(0, bar1);
    lcd_.createChar(1, bar2);
    lcd_.createChar(2, bar3);
    lcd_.createChar(3, bar4);
    lcd_.createChar(4, bar5);
    lcd_.createChar(5, arrow);
}

void Display::begin() {
    Wire.begin();
    lcd_.init();
    lcd_.backlight();
    defineCustomChars();
}

void Display::render(const ScreenBuffer &screen) {
    for (uint8_t row = 0; row < 2; ++row) {
        const char *src = (row == 0) ? screen.line1 : screen.line2;
        char *old = (row == 0) ? last_.line1 : last_.line2;

        for (uint8_t col = 0; col < 16; ++col) {
            if (!hasLast_ || old[col] != src[col]) {
                lcd_.setCursor(col, row);
                lcd_.write(static_cast<uint8_t>(src[col]));
                old[col] = src[col];
            }
        }
    }
    hasLast_ = true;
}
