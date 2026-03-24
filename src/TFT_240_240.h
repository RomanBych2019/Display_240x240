#pragma once

#include "Arduino.h"
#include "can_settings.h"
#include <PNGdec.h>
#include "SPI.h"
#include <TFT_eSPI.h>
#include "PNG_FS_Support.h"
#include <LittleFS.h>
#include <freertos/task.h>

#include <vector>
#include "ArialRoundedMTBold20.h"
#include "ArialRoundedMTBold24.h"
#include "ArialRoundedMTBold40.h"
#include "ArialRoundedMTBold45.h"

#include "SolenoidHealth.h"
#include "DisplayConfig.h"

class TFT_240_240
{
public:
    struct Scale
    {
        int x_, y_, l_, h_, vol_;
        int vertical_ = true;
        String label_beg_ = "";
        String label_end_ = "";

        Scale(int x, int y, int l, int h, bool vertical)
            : x_(x), y_(y), l_(l), h_(h), vol_(0), vertical_(vertical) {}

        void setLabel(const String &beg, const String &end)
        {
            label_beg_ = beg;
            label_end_ = end;
        }

        void setVolume(int vol)
        {
            vol_ = vol;
        }

        void show(TFT_eSprite &spr);
    };

public:
    // explicit TFT_240_240(fs::LittleFSFS& fs);
    explicit TFT_240_240();

    int getScreenNowShow() const;
    void pngShow(const String &str);
    void backLight(bool vol);
    // void setData(const Can_Data &dat);
    void setData(const Can_Data &dat, const Valve valve[]);
    void update();

    void setDisplayConfig(const DisplayConfig &cfg);
    void nextUserPage();
    void prevUserPage();
    void showPage(DisplayPageId id);
    void updatePageOrder();

private:
    struct dot
    {
        int16_t x;
        int16_t y;
        int fill;
    };

    Valve valve_[4] = {{SolenoidHealth::OK, 0},
                       {SolenoidHealth::OK, 0},
                       {SolenoidHealth::OK, 0},
                       {SolenoidHealth::OK, 0}}; // массив для хранения состояния клапанов баллонов

    static constexpr int MAX_BRIGHT = 150;
    static constexpr int MAX_IMAGE_WIDTH = 240;
    static constexpr unsigned int KG_TO_M3 = 1;

    static TFT_240_240 *activeInstance_;
    static void pngDrawCallback(PNGDRAW *pDraw);

    const int Strok_UP_Y = 55;
    const int Strok_DOWN_Y = 130;

    Can_Data data{};
    int color = 0;
    bool flag_change_screen = false;

    TFT_eSPI tft;
    TFT_eSprite spr;
    TFT_eSprite buttonOnOff;
    PNG png;

private:
    int set_color(int vol) const;
    std::vector<dot> CircularScale(uint8_t quantity, int16_t start, int16_t end,
                                   int16_t x, int16_t y, int16_t r, uint8_t vol) const;

    void firstOn();

    void screen_LLS();
    void screen_ButtonOn();
    void screen_paramEVO();
    void screen_CNG();
    void screen_EVOLost();
    void screen_ValveTank();

    void colorCircle(int color);
    void scale_vertical(int vol);
    void scale_horizont(int vol);

    DisplayConfig displayConfig_;
    int currentUserPageIndex_ = 0;
    // void loadUserPages();

    uint16_t getColor(uint8_t red, uint8_t green, uint8_t blue) const;
    bool true_can() const;
};