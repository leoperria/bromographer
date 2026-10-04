#pragma once

#include <LiquidCrystal_I2C.h>

#include "state_machine.h"

class Display {
public:
    Display();

    void begin();
    void render(const ScreenBuffer& screen);

private:
    void defineCustomChars();

    LiquidCrystal_I2C lcd_;
    ScreenBuffer last_{};
    bool hasLast_ = false;
};

