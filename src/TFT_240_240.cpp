#include "TFT_240_240.h"

TFT_240_240 *TFT_240_240::activeInstance_ = nullptr;

void TFT_240_240::pngDrawCallback(PNGDRAW *pDraw)
{
    // if (!activeInstance_)
    //     return;

    // uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    // activeInstance_->png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    // activeInstance_->spr.pushImage(0, pDraw->y, pDraw->iWidth, 1, lineBuffer);

    if (!activeInstance_)
        return;

    static uint16_t lineBuffer[MAX_IMAGE_WIDTH];

    activeInstance_->png.getLineAsRGB565(
        pDraw,
        lineBuffer,
        PNG_RGB565_BIG_ENDIAN,
        0xffffffff);

    activeInstance_->spr.pushImage(0, pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

void TFT_240_240::Scale::show(TFT_eSprite &spr)
{
    spr.setTextColor(TFT_DARKGREY);
    spr.drawRightString("F", x_, y_, 4);
    spr.drawRightString("E", x_, y_ + h_, 4);

    if (vertical_)
    {
        for (int i = 0; i < 9; i++)
        {
            if (i == 4)
                spr.fillRoundRect(x_ + 10, y_ + 10 + 8 * i, l_ / 2, 3, 1, TFT_WHITE);
            else
                spr.fillRoundRect(36, 64 + 8 * i, 10, 2, 1, TFT_WHITE);
        }

        int higttY = constrain(map(vol_, 5, 200, 192, 48), 48, 192);
        int lengt = constrain(map(vol_, 5, 200, 1, 143), 1, 143);
        int color = vol_ < 30 ? TFT_RED : TFT_DARKGREEN;

        spr.fillRoundRect(48, higttY, 12, lengt, 2, color);

        for (int i = 0; i < 9; i++)
        {
            spr.fillRoundRect(48, 64 + 8 * i, 10, 2, 1, TFT_BLACK);
        }
    }
}

TFT_240_240::TFT_240_240()
    : tft(), spr(&tft), buttonOnOff(&tft)
{
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);

    spr.begin();
    spr.setTextSize(1);

    firstOn();
}

int TFT_240_240::set_color(int vol) const
{
    if (vol > 70)
        return TFT_GREEN;
    else if (vol > 40)
        return TFT_ORANGE;
    else if (vol >= 0)
        return TFT_RED;
    else
        return TFT_DARKGREY;
}

std::vector<TFT_240_240::dot> TFT_240_240::CircularScale(uint8_t quantity, int16_t start, int16_t end,
                                                         int16_t x, int16_t y, int16_t r, uint8_t vol) const
{
    if (quantity < 2)
        return {};

    std::vector<dot> v{};
    dot d1{0, 0, 0};
    double alfa = 1.0 * (end - start) / (quantity - 1);

    for (int i = 0; i < quantity; i++)
    {
        auto c = radians((start - alfa * i));
        d1.x = (x + round(r * cos(c)));
        d1.y = (y + round(r * sin(c)));
        d1.fill = vol >= i * 100.0 / (quantity - 1) ? 1 : 0;
        v.push_back(d1);
    }
    return v;
}

void TFT_240_240::firstOn()
{
    spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
    spr.fillSprite(TFT_BLACK);
    pngShow("/screen0.png");
    spr.pushSprite(0, 0, TFT_TRANSPARENT);
    spr.deleteSprite();
    backLight(true);
    spr.setColorDepth(8);
}

void TFT_240_240::screen_LLS()
{
    spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
    spr.fillScreen(TFT_BLACK);

    spr.drawSmoothArc(125, 61, 7, 9, 180, 0, TFT_DARKGREY, TFT_BLACK, true);
    spr.fillRoundRect(112, 50, 16, 28, 4, TFT_DARKGREY);
    spr.fillRect(108, 75, 24, 5, TFT_DARKGREY);
    spr.fillRect(115, 52, 10, 10, TFT_BLACK);

    spr.setTextColor(TFT_DARKGREY);
    spr.loadFont(ArialRoundedMTBold24);
    spr.setTextDatum(TC_DATUM);
    spr.drawString("oil", 120, 85);

    float sx = 0, sy = 1;
    uint16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0;

    for (int i = 10; i <= 170; i += 20)
    {
        sx = cos((-1 * i) * 0.0174532925);
        sy = sin((-1 * i) * 0.0174532925);
        x0 = sx * 114 + 120;
        yy0 = sy * 114 + 120;
        x1 = sx * 100 + 120;
        yy1 = sy * 100 + 120;

        spr.drawLine(x0, yy0, x1, yy1, TFT_GREEN);
    }

    for (int i = 10; i <= 170; i += 10)
    {
        sx = cos((-1 * i) * 0.0174532925);
        sy = sin((-1 * i) * 0.0174532925);
        x0 = sx * 102 + 120;
        yy0 = sy * 102 + 120;
        spr.drawPixel(x0, yy0, TFT_WHITE);

        if (i == 10 || i == 170)
            spr.fillCircle(x0, yy0, 2, TFT_WHITE);
        if (i == 90 || i == 270)
            spr.fillCircle(x0, yy0, 2, TFT_WHITE);
    }

    int x = TFT_WIDTH / 2;
    String str{};

    spr.loadFont(ArialRoundedMTBold24);
    spr.setTextDatum(TC_DATUM);
    spr.setTextColor(TFT_WHITE);
    str = "E                                 F";
    spr.drawString(str, 120, 115);
    spr.setTextColor(TFT_DARKGREY);
    str = " " + String(data.full_tank) + " L";
    spr.drawString(str, x, 205);

    if (data.lls == -1 || data.lls / 10 >= data.full_tank)
    {
        spr.setTextColor(TFT_RED);
        str = "Sensor lost";
        spr.loadFont(ArialRoundedMTBold24);
    }
    else if (data.lls / 10 <= data.full_tank)
    {
        float sdeg = map(data.lls / 10, 0, data.full_tank, -168, -12);
        float sxNeedle = 90 * cos(sdeg * 0.0174532925) + 120;
        float syNeedle = 90 * sin(sdeg * 0.0174532925) + 120;

        float sx1 = 15 * cos((sdeg - 20) * 0.0174532925) + 120;
        float sy1 = 15 * sin((sdeg - 20) * 0.0174532925) + 120;

        float sx2 = 15 * cos((sdeg + 20) * 0.0174532925) + 120;
        float sy2 = 15 * sin((sdeg + 20) * 0.0174532925) + 120;

        spr.fillTriangle(sxNeedle, syNeedle, sx1, sy1, 120, 120, TFT_RED);
        spr.fillTriangle(sxNeedle, syNeedle, sx2, sy2, 120, 120, TFT_RED);
        spr.fillCircle(120, 120, 10, TFT_DARKGREY);

        spr.setTextColor(TFT_GREEN);
        str = String(data.lls / 10.0, 1);
        spr.loadFont(ArialRoundedMTBold45);
    }
    else
    {
        spr.setTextColor(TFT_RED);
        str = "Sensor lost";
        spr.loadFont(ArialRoundedMTBold24);
    }

    spr.drawString(str, x, 150);
    spr.unloadFont();
    spr.pushSprite(0, 0);
    spr.deleteSprite();
}

void TFT_240_240::screen_ButtonOn()
{
    spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
    spr.fillScreen(TFT_BLACK);

    if (data.state)
        pngShow("/screen11.png");
    else
        pngShow("/screen10.png");

    scale_vertical(data.cngLevel / 2);
    spr.pushSprite(0, 0);
    spr.deleteSprite();
}

void TFT_240_240::screen_paramEVO()
{
    const float tred = 1.0 * data.cngWaterTemperature;
    const float presturbo = 0.01 * data.cngTurboPressure;
    const float presred = 0.02 * data.cngRailPressure;
    const int prescng = data.cngLevel;

    const float injection_time = 0.1 * data.cngInjectionTime;
    const float CNG = 0.1 * data.cngIstValue;
    const String id = "id " + String(data.verID) + "  fw " + String(data.verFMlow);

    spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
    spr.fillScreen(TFT_BLACK);

    pngShow("/screen2.png");

    if (true_can())
    {
        spr.loadFont(ArialRoundedMTBold20);

        spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        spr.setTextDatum(TC_DATUM);
        spr.drawString(id, 120, 10);

        spr.setTextColor(TFT_DARKCYAN, TFT_BLACK);
        spr.setTextDatum(TR_DATUM);
        String str = String(tred, 1);
        spr.drawString(str, 106, 38);
        str = String(presred, 2);
        spr.drawString(str, 106, 88);
        str = String(injection_time, 1);
        spr.drawString(str, 106, 138);

        spr.setTextDatum(TL_DATUM);
        str = String(presturbo, 2);
        spr.drawString(str, 139, 38);
        str = String(prescng);
        spr.drawString(str, 139, 88);
        str = String(CNG, 1);
        spr.drawString(str, 139, 138);

        spr.setTextColor(TFT_ORANGE, TFT_BLACK);
        str.clear();
        str = String(data.errorEVO[0]) + " ";
        str += String(data.errorEVO[1]) + " ";
        str += String(data.errorEVO[2]) + " ";
        str += String(data.errorEVO[3]) + " ";
        str += String(data.errorEVO[4]) + " ";
        str += String(data.errorEVO[5]);

        spr.setTextDatum(TC_DATUM);
        spr.drawString(str, 120, 190);
        spr.unloadFont();
    }

    spr.pushSprite(0, 0);
    spr.deleteSprite();
}

void TFT_240_240::screen_ValveTank()
{
    spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
    spr.fillScreen(TFT_BLACK);

    uint32_t color_[4]{};
    for (int i = 0; i < 4; i++)
    {
        if (valve_[i].getState().heat == SolenoidHealth::OK)
            color_[i] = TFT_DARKGREEN;
        else if (valve_[i].getState().heat == SolenoidHealth::SHORT)
            color_[i] = TFT_RED;
        else if (valve_[i].getState().heat == SolenoidHealth::OVERLOAD)
            color_[i] = TFT_MAROON;
        else if (valve_[i].getState().heat == SolenoidHealth::OPEN_CIRCUIT)
            color_[i] = TFT_ORANGE;
        else if (valve_[i].getState().heat == SolenoidHealth::MOSFET_LEAK_OR_SHORT)
            color_[i] = TFT_LIGHTGREY;
    }

    spr.drawSmoothArc(120, 120, 100, 120, 10, 90, color_[2], TFT_BLACK, true);
    spr.drawSmoothArc(120, 120, 100, 120, 100, 180, color_[3], TFT_BLACK, true);
    spr.drawSmoothArc(120, 120, 100, 200, 190, 270, color_[0], TFT_BLACK, true);
    spr.drawSmoothArc(120, 120, 100, 120, 280, 360, color_[1], TFT_BLACK, true);

    // spr.fillRect(0, 118, 240, 8, TFT_BLACK);
    // spr.fillRect(118, 0, 8, 240, TFT_BLACK);

    for (int i = 0; i < 5; i++)
    {
        spr.drawCircle(120, 120, 100 + i * 4, TFT_BLACK);
        spr.drawCircle(120, 120, 100 + i * 4, TFT_BLACK);
    }

    spr.setTextColor(TFT_DARKGREY);
    spr.loadFont(ArialRoundedMTBold20);
    spr.setTextDatum(TC_DATUM);
    spr.drawString("current, A", 120, 50);

    spr.setTextColor(TFT_DARKGREY);
    spr.loadFont(ArialRoundedMTBold24);

    spr.drawString("sol #1  " + String(valve_[0].getState().current / 100.0, 1), 120, 85);
    spr.drawString("sol #2  " + String(valve_[1].getState().current / 100.0, 1), 120, 115);
    spr.drawString("sol #3  " + String(valve_[2].getState().current / 100.0, 1), 120, 145);
    spr.drawString("sol #4  " + String(valve_[3].getState().current / 100.0, 1), 120, 175);

    spr.unloadFont();
    spr.pushSprite(0, 0);
    spr.deleteSprite();
}

void TFT_240_240::screen_CNG()
{
    int x = 84;
    int y = Strok_DOWN_Y;

    spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
    spr.fillScreen(TFT_BLACK);
    pngShow("/screen4.png");

    String str{};
    if (data.state == 0)
        str = "Disel";

    scale_vertical(data.cngLevel / 2);
    scale_horizont(data.cngIstValue / 10.0);

    if (true_can())
    {
        String res{};

        if (str != "Disel")
        {
            x = 202;
            y = 125;
            spr.setTextColor(TFT_SKYBLUE);
            if (data.average_gasconsumption)
                res = String(data.average_gasconsumption, 0);
            else
                res = "";
            colorCircle(color);
        }
        else
        {
            res = "";
        }

        spr.loadFont(ArialRoundedMTBold24);
        spr.drawString(res, x, y);
        spr.unloadFont();

        x = 84;
        y = 170;
        spr.loadFont(ArialRoundedMTBold40);
        spr.setTextColor(TFT_DARKGREY);
        if (str == "Disel")
        {
            res = str;
        }
        else
        {
            if (data.average_gasconsumption)
                res = String(data.distLPG.gas_mileage, 0);
            else
                res = "";
        }
        spr.setTextDatum(TL_DATUM);
        spr.drawString(res, x, y);
        spr.unloadFont();

        spr.loadFont(ArialRoundedMTBold45);
        spr.setTextDatum(TC_DATUM);
        String s = String(data.ACC);

        if (str == "Disel")
            spr.setTextColor(TFT_DARKGREY);
        else
            spr.setTextColor(color);

        if (data.cruise == 1)
            s = String(data.engineLoad);

        if (data.ACC == 255)
            s = "---";

        spr.drawString(s, 120, 24);
    }

    spr.pushSprite(0, 0);
    spr.unloadFont();
    spr.deleteSprite();
}

void TFT_240_240::screen_EVOLost()
{
    spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
    spr.fillScreen(TFT_BLACK);
    colorCircle(TFT_RED);

    int x = 120;
    int y = 105;

    spr.setTextColor(TFT_RED);
    spr.loadFont(ArialRoundedMTBold40);
    spr.setTextDatum(TC_DATUM);
    spr.drawString("EVO lost", x, y);
    spr.pushSprite(0, 0);
    spr.unloadFont();
    spr.deleteSprite();
}

void TFT_240_240::colorCircle(int colorValue)
{
    for (int i = 0; i < 4; i++)
        spr.drawCircle(120, 120, 120 - i, colorValue);
}

void TFT_240_240::scale_vertical(int vol)
{
    int higttY = constrain(map(vol, 5, 100, 192, 48), 48, 192);
    int lengt = constrain(map(vol, 5, 100, 1, 143), 1, 143);
    int colorValue = vol < 30 ? TFT_RED : TFT_DARKGREEN;

    spr.fillRoundRect(52, 49, 12, 142, 2, getColor(50, 50, 50));
    spr.fillRoundRect(52, higttY, 12, lengt, 2, colorValue);

    for (int i = 0; i < 9; i++)
        spr.fillRoundRect(52, 63 + 14 * i, 13, 2, 0, TFT_BLACK);
}

void TFT_240_240::scale_horizont(int vol)
{
    int ll = constrain(map(vol, 0, 25, 0, 143), 0, 143);
    spr.fillRoundRect(82, 107, 143, 12, 2, getColor(50, 50, 50));
    spr.fillRoundRect(82, 107, ll, 12, 2, TFT_SKYBLUE);

    for (int i = 0; i < 9; i++)
        spr.fillRoundRect(97 + i * 14, 106, 2, 13, 0, TFT_BLACK);
}

uint16_t TFT_240_240::getColor(uint8_t red, uint8_t green, uint8_t blue) const
{
    red >>= 3;
    green >>= 2;
    blue >>= 3;
    return (red << 11) | (green << 5) | blue;
}

bool TFT_240_240::true_can() const
{
    return !(data.state == 0 &&
             data.cngWaterTemperature == 0 &&
             data.cngTurboPressure == 0 &&
             data.cngRailPressure == 0 &&
             data.cngLevel == 0 &&
             data.cngInjectionTime == 0 &&
             data.cngIstValue == 0);
}

int TFT_240_240::getScreenNowShow() const
{
    // return screenNowShow;
}

void TFT_240_240::pngShow(const String &path)
{
    // activeInstance_ = this;
    // int16_t rc = png.open(str.c_str(), pngOpen, pngClose, pngRead, pngSeek, pngDrawCallback);
    // if (rc == PNG_SUCCESS)
    //     png.decode(nullptr, 1);
    // png.close();
    // activeInstance_ = nullptr;

    activeInstance_ = this;
    int16_t rc = png.open(
        path.c_str(),
        pngOpen,
        pngClose,
        pngRead,
        pngSeek,
        pngDrawCallback);

    if (rc != PNG_SUCCESS)
    {
        Serial.printf("png.open failed: %d, file: %s\n", rc, path.c_str());
        activeInstance_ = nullptr;
        return;
    }

    rc = png.decode(nullptr, 1);
    if (rc != PNG_SUCCESS)
    {
        Serial.printf("png.decode failed: %d, file: %s\n", rc, path.c_str());
    }

    png.close();
    activeInstance_ = nullptr;
}

void TFT_240_240::backLight(bool vol)
{
    if (vol)
    {
        if (currentUserPageIndex_ != static_cast<int>(DisplayPageId::EVOLost))
        {
            for (int i = 0; i < MAX_BRIGHT; i++)
            {
                analogWrite(TFT_BL, i);
                delay(1);
                yield();
            }
        }
        else
        {
            analogWrite(TFT_BL, 20);
        }
    }
    else
    {
        for (int i = MAX_BRIGHT; i != 0; i--)
        {
            analogWrite(TFT_BL, i);
            delay(1);
            yield();
        }
    }
}

// void TFT_240_240::setData(const Can_Data &dat)
// {
//     data = dat;
// }

void TFT_240_240::setData(const Can_Data &dat, const Valve valve[])
{
    data = dat;
    for (int i = 0; i < 4; i++)
        valve_[i] = valve[i];
}

// void TFT_240_240::setScreenNumber(int screenNumber)
// {
//     if (flag_change_screen)
//         return;

//     screenNowShow = screen(screenNumber);
// }

void TFT_240_240::update()
{
    color = set_color(data.levelEconomicalDriving);
}

void TFT_240_240::setDisplayConfig(const DisplayConfig &cfg)
{
    displayConfig_ = cfg;
    currentUserPageIndex_ = 0; // Обнуляем индекс текущей страницы
}

void TFT_240_240::nextUserPage()
{
    DisplayPageId pages[DISPLAY_PAGE_COUNT];
    int count = 0;

    for (uint8_t order = 0; order < 250 && count < DISPLAY_PAGE_COUNT; ++order)
    {
        for (const auto &p : displayConfig_.pages)
        {
            if (!p.systemPage && p.enabled && p.order == order)
            {
                pages[count++] = p.id;
            }
        }
    }

    if (count <= 0)
        return;

    currentUserPageIndex_++;
    if (currentUserPageIndex_ >= count)
        currentUserPageIndex_ = 0;
    showPage(pages[currentUserPageIndex_]);
}

void TFT_240_240::prevUserPage()
{
    DisplayPageId pages[DISPLAY_PAGE_COUNT];
    int count = 0;

    for (uint8_t order = 0; order < 250 && count < DISPLAY_PAGE_COUNT; ++order)
    {
        for (const auto &p : displayConfig_.pages)
        {
            if (!p.systemPage && p.enabled && p.order == order)
            {
                pages[count++] = p.id;
            }
        }
    }

    if (count <= 0)
        return;

    currentUserPageIndex_--;
    if (currentUserPageIndex_ < 0)
        currentUserPageIndex_ = count - 1;
    showPage(pages[currentUserPageIndex_]);
}

void TFT_240_240::showPage(DisplayPageId id)
{
    backLight(false);
    switch (id)
    {
    case DisplayPageId::CNG:
        // log_e("screen_CNG");
        screen_CNG();
        break;
    case DisplayPageId::ParamEVO:
        // log_e("ParamEVO");
        screen_paramEVO();
        break;
    case DisplayPageId::ButtonOn:
        // log_e("ButtonOn");
        screen_ButtonOn();
        break;
    case DisplayPageId::LSLevel:
        // log_e("screen_LLS");
        screen_LLS();
        break;
    case DisplayPageId::ValveTank:
        screen_ValveTank();
        break;
    case DisplayPageId::EVOLost:
        // log_e("screen_EVOLost");
        screen_EVOLost();
        break;
    default:
        break;
    }
    backLight(true);
}

void TFT_240_240::updatePageOrder()
{
    for (int i = 0; i < DISPLAY_PAGE_COUNT; ++i)
    {
        if (displayConfig_.pages[i].enabled)
        {
            displayConfig_.pages[i].order = i;
        }
    }
}