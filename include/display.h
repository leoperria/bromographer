#pragma once

#include <U8g2lib.h>

#include "state_machine.h"

class Display {
public:
    Display();

    void begin();
    void render(const ScreenBuffer& screen);

private:
    static constexpr uint8_t kCellWidthPx = 8;
    static constexpr uint8_t kCellHeightPx = 13;
    static constexpr uint8_t kLine1TopPx = 8;
    static constexpr uint8_t kLine2TopPx = 34;

    void drawCellGlyph(char c, uint8_t col, uint8_t rowTopPx);
    void drawTextLine(const char* line, uint8_t rowTopPx);

    U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled_;
    ScreenBuffer last_{};
    bool hasLast_ = false;
};

