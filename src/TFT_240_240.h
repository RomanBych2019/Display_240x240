#pragma once
#include "Arduino.h"
#include "can_settings.h"
#include <PNGdec.h>
#include "SPI.h"
#include <TFT_eSPI.h> // Hardware-specific library
#include "PNG_FS_Support.h"
#include <vector>

#define MAX_BRIGHT 200
#define MAX_IMAGE_WIDTH 240

#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG(x) Serial.println(x)
#else
#define DEBUG(x)
#endif

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft); // Создаем объект sprite для работы с изображением на экране

const unsigned int KG_TO_M3 = 1;                    // коэф переводка  газа из кг в м3

void pngDraw(PNGDRAW *pDraw)
{
    int16_t xpos = 0;
    int16_t ypos = 0;
    uint16_t lineBuffer[MAX_IMAGE_WIDTH];
    png.getLineAsRGB565(pDraw, lineBuffer, PNG_RGB565_BIG_ENDIAN, 0xffffffff);
    spr.pushImage(xpos, ypos + pDraw->y, pDraw->iWidth, 1, lineBuffer);
}

class TFT_240_240
{

private:

    struct dot
    {
        int16_t x;
        int16_t y;
        int fill;
    };

    Can_Data const *data{};
    int screenNowShow = 0;

    int set_color(int vol)
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

    std::vector<dot> CircularScale(uint8_t quantity, int16_t start, int16_t end, int16_t x, int16_t y, int16_t r, uint8_t vol)
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
            // Serial.println("Corner: " + String(c) + " cos: " + String(cos(c)) + " sin: " + String(sin(c)));
            // Serial.println("X: " + String(d1.x) + " Y: " + String(d1.y));

            d1.fill = vol >= i * 100.0 / (quantity - 1) ? 1 : 0;
            v.push_back(d1);
        }
        return v;
    }

    void firstOn()
    {
        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillSprite(TFT_BLACK);
        pngShow("/screen0.png");
        spr.pushSprite(0, 0, TFT_TRANSPARENT);
        spr.deleteSprite();
        backLight(true);
    }

     //  экран кнопки включения
     void screen_ButtonOn(int status)
     {
         spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
         spr.fillScreen(TFT_BLACK);
 
         if (status)
             pngShow("/screen11.png");
         else
             pngShow("/screen10.png");
 
         spr.pushSprite(0, 0, TFT_TRANSPARENT);
         spr.deleteSprite();
         screenNowShow = 0;
 
         DEBUG("Screen:" + String(screenNowShow));
     }
 
    //  экран вывода акселератора и среднего расхода
    void screen_ACC(int map_level, int ACC, String str = "")
    {
        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillScreen(TFT_BLACK);

        pngShow("/screen1.png");

        ACC = constrain(ACC, 0, 100);
        int color = set_color(map_level);
        if (str == "Disel")
            color = TFT_DARKGREY;
        for (int i = 0; i < 6; i++)
            spr.drawCircle(120, 120, 120 - i, color);
        // шкала средний расход
        if (str.toInt())
        {
            int ll = constrain(map(str.toInt(), 0, 40, 2, 150), 0, 150);
            spr.fillRoundRect(44, 170, ll, 4, 2, TFT_SKYBLUE);
        }

        // нижняя строка
        int font = 4;
        int x = 120;
        int y = 135;
        if (str == "EVO lost")
            spr.setTextColor(TFT_RED);
        else if (str == "Disel")
            spr.setTextColor(TFT_DARKGREY);
        else
        {
            font = 6;
            y = 120;
            spr.setTextColor(TFT_DARKGREY);
        }
        spr.drawCentreString(str, x, y, font);

        // верхняя строка
        String s = String(ACC);
        spr.setTextColor(color);
        if (str == "Cruise")
            spr.drawCentreString(str, x, 45, 6);
        else if (str != "EVO lost")
            spr.drawCentreString(s, x, 50, 6);
        spr.pushSprite(0, 0, TFT_TRANSPARENT);
        spr.deleteSprite();

        screenNowShow = 1;

        DEBUG("Screen:" + String(screenNowShow));
    }

    // экран вывода параметров
    void screen_EVO(float tred, float presturbo, float presred, int prescng, uint8_t const alarm[], float injection_time, float CNG, String id)
    {
        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillScreen(TFT_BLACK);
        pngShow("/screen2.png");
        int font = 4;
        spr.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
        spr.drawCentreString(id, 120, 10, 2);
        spr.setTextColor(TFT_DARKCYAN, TFT_BLACK);

        // вывод Тредуктора + вывод Давление турбо
        String str = String(tred, 1);
        spr.drawRightString(str, 106, 33, font);
        str = String(presturbo, 2);
        spr.drawString(str, 139, 33, font);

        // вывод Давление редуктор + вывод Уровень
        str = String(presred, 2);
        spr.drawRightString(str, 106, 83, font);
        str = String(prescng);
        spr.drawString(str, 139, 83, font);

        // вывод Впрыск + вывод CNG
        str = String(injection_time, 1);
        spr.drawRightString(str, 106, 133, font);
        str = String(CNG, 1) + "   ";
        spr.drawString(str, 139, 133, font);

        // вывод флагов ошибок EVO
        spr.setTextColor(TFT_ORANGE, TFT_BLACK);
        if (alarm)
        {
            String str{};
            str = String(alarm[0]) + " ";
            str += String(alarm[1]) + " ";
            str += String(alarm[2]) + " ";
            str += String(alarm[3]) + " ";
            str += String(alarm[4]) + " ";
            str += String(alarm[5]);
            spr.drawCentreString(str, 120, 183, font);
        }
        spr.pushSprite(0, 0, TFT_TRANSPARENT);
        spr.deleteSprite();
        screenNowShow = 2;

        DEBUG("Screen:" + String(screenNowShow));
    }

    //  экран вывода уровня газа и оставшегося пробега
    void screen_CNGLevel(int map_level, int cngLevel, String distance = "", String str = "")
    {
        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillScreen(TFT_BLACK);
        cngLevel = constrain(cngLevel / 2, 0, 100);
        int color = set_color(map_level);
        pngShow("/screen3.png");
        if (str == "Disel")
            color = TFT_DARKGREY;
        for (int i = 0; i < 6; i++)
            spr.drawCircle(120, 120, 120 - i, color);

        // полукруглый индикатор уровня
        auto v = CircularScale(9, 200, 60, 120, 120, 85, cngLevel);
        for (auto coord : v)
            if (coord.fill)
            {
                spr.drawSmoothCircle(coord.x, coord.y, 6, TFT_DARKGREY, TFT_BLACK);
                spr.fillCircle(coord.x, coord.y, 6, TFT_CYAN);
            }
            else
                spr.drawSmoothCircle(coord.x, coord.y, 6, TFT_DARKGREY, TFT_BLACK);

        // шкала средний расход
        if (str.toInt())
        {
            int ll = constrain(map(str.toInt(), 0, 40, 2, 150), 0, 150);
            spr.fillRoundRect(44, 170, ll, 4, 2, TFT_SKYBLUE);
        }

        // нижняя строка
        int font = 4;
        int x = 120;
        int y = 135;
        if (str == "EVO lost")
            spr.setTextColor(TFT_RED);
        else if (str == "Disel")
            spr.setTextColor(TFT_DARKGREY);
        else
        {
            font = 6;
            y = 120;
            spr.setTextColor(TFT_DARKGREY);
        }
        spr.drawCentreString(str, x, y, font);

        // верхняя строка
        String s = String(cngLevel);
        spr.setTextColor(color);
        if (str != "EVO lost")
            spr.drawCentreString(s, x, 50, 6);
        spr.pushSprite(0, 0, TFT_TRANSPARENT);
        spr.deleteSprite();
        screenNowShow = 3;

        DEBUG("Screen:" + String(screenNowShow));
    }

    //  экран потери связи с газовым ЭБУ
    void screen_EVOLost(String str = "")
    {
        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillScreen(TFT_BLACK);
        for (int i = 0; i < 6; i++)
            spr.drawCircle(120, 120, 120 - i, TFT_RED);
        int font = 4;
        int x = 120;
        int y = 105;
        if (str == "EVO lost")
            spr.setTextColor(TFT_RED);
        spr.drawCentreString(str, x, y, font);
        spr.pushSprite(0, 0, TFT_TRANSPARENT);
        spr.deleteSprite();
        screenNowShow = 4;

        DEBUG("Screen:" + String(screenNowShow));
    }

public:
    TFT_240_240()
    {
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, LOW);
        spr.begin();
        spr.setTextSize(1);
        spr.setColorDepth(16);
        firstOn();
        spr.setColorDepth(8);
    }

    int getScreenNowShow()
    {
        return screenNowShow;
    }

    void pngShow(String str)
    {
        int16_t rc = png.open(str.c_str(), pngOpen, pngClose, pngRead, pngSeek, pngDraw);
        rc = png.decode(NULL, 0);
        png.close();
    }

    void backLight(boolean vol)
    {
        if (vol)
        {
            for (int i = 0; i < MAX_BRIGHT; i++)
            {
                analogWrite(TFT_BL, i);
                delay(10);
            }
        }
        else
        {
            for (int i = MAX_BRIGHT; i != 0; i--)
            {
                analogWrite(TFT_BL, i);
                delay(10);
            }
        }
    }

    void updateData(const Can_Data *dat)
    {
        data = dat;
    }

    void updateScreen(int screenNumber)
    {
        switch (screenNumber)
        {
        case 0:
            screen_ButtonOn(data->state);
            break;
        case 1:
            if (data->state == 1)
            {
                if (data->wheelSpeed > 20)
                {
                    if (data->cruise == 0)
                    {
                        if (data->distLPG.result > 1000)
                            screen_ACC(data->levelEconomicalDriving, data->ACC, String(data->cngTripFuel * 100.0 / data->distLPG.result, 1));
                        else
                            screen_ACC(data->levelEconomicalDriving, data->ACC);
                    }
                    else if (data->engineLoad < 10)
                        screen_ACC(0, 0, "Cruise");
                    else if (data->engineLoad < 25)
                        screen_ACC(50, 0, "Cruise");
                    else if (data->engineLoad < 50)
                        screen_ACC(100, 0, "Cruise");
                    else if (data->engineLoad < 65)
                        screen_ACC(50, 0, "Cruise");
                    else
                        screen_ACC(0, -1);
                }
                else
                {
                    screen_ACC(0, data->ACC);
                }
            }
            else
            {
                screen_ACC(0, data->ACC, "Disel");
            }
            break;

        case 3:
            screen_EVO(1.0 * (data->cngWaterTemperature),
                     0.01 * data->cngTurboPressure,
                     0.02 * data->cngRailPressure,
                     data->cngLevel, data->errorEVO,
                     0.1 * data->cngInjectionTime,
                     0.1 * data->cngIstValue,
                     "id " + String(data->verID) + "  fw " + String(data->verFMlow));
            break;

        case 2:
            if (data->state == 1)
            {
                if (data->distLPG.result > 1000 && data->cngTripFuel > 0)
                    screen_CNGLevel(data->levelEconomicalDriving, data->cngLevel, String((data->cngLevel * data->tankVolume) / (200 * KG_TO_M3) * data->distLPG.result / data->cngTripFuel, 1));
                else
                screen_CNGLevel(data->levelEconomicalDriving, data->cngLevel, String((data->cngLevel * data->tankVolume / 200) / 18 * 100, 1));
            }
            else
            {
                screen_CNGLevel(data->levelEconomicalDriving, data->cngLevel, String((data->cngLevel * data->tankVolume / 200) / 18 * 100, 1), "Disel");
            }
            break;

        case 4:
            screen_EVOLost("EVO lost");
            break;
        default:
            break;
        }
    }
};