#pragma once
#include <Arduino.h>

enum class DisplayPageId : uint8_t
{
    ButtonOn = 0,
    CNG,
    ParamEVO,
    LSLevel,
    ValveTank,
    EVOLost,
};

struct DisplayPageConfig
{
    DisplayPageId id;
    const char* key;
    const char* title;
    bool enabled;
    uint8_t order;
    bool systemPage;
};

static constexpr uint8_t DISPLAY_PAGE_COUNT = 6;

struct DisplayConfig
{
    DisplayPageConfig pages[DISPLAY_PAGE_COUNT];
};