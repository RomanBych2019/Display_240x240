#pragma once
#include "Arduino.h"
#include "can_settings.h"
#include <PNGdec.h>
#include "SPI.h"
#include <TFT_eSPI.h> // Hardware-specific library
#include "PNG_FS_Support.h"
#include <freertos/task.h>

#include <vector>
#include "ArialRoundedMTBold20.h"
#include "ArialRoundedMTBold24.h"
#include "ArialRoundedMTBold40.h"
#include "ArialRoundedMTBold45.h"
// #include "ArialRoundedMTBold50.h"

#define MAX_BRIGHT 150
#define MAX_IMAGE_WIDTH 240

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);       // Создаем объект sprite для работы с изображением на экране
TFT_eSprite butoOnOff = TFT_eSprite(&tft); // Создаем объект sprite для работы с изображением на экране

const unsigned int KG_TO_M3 = 1; // коэф переводка  газа из кг в м3

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
public:
    //   номера экранов с выводом данных на дисплей
    enum screen
    {
        Non = -1,
        CNG = 0,
        paramEVO = 1,
        ButtonOn = 2,
        LSLevel = 3,
        Acc = 4,
        EVOLost = 5,
    };

private:
    struct dot
    {
        int16_t x;
        int16_t y;
        int fill;
    };

    const int Strok_UP_Y = 55;
    const int Strok_DOWN_Y = 130;
    Can_Data data{};
    screen screenNowShow = screen::Non;
    screen screenNowShowOld = screen::Non;
    int color = 0;
    bool flag_change_screen = false;

    fs::LittleFSFS filesystem;

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
        backLight(TRUE);
        spr.setColorDepth(8);
    }

    //  экран вывода  уровня топлива
    void screen_LLS()
    {
        // uint32_t dt = millis();

        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillScreen(TFT_BLACK);

        {
            {
                spr.drawSmoothArc(125, 61, 7, 9, 180, 0, TFT_DARKGREY, TFT_BLACK, true);
                spr.fillRoundRect(112, 50, 16, 28, 4, TFT_DARKGREY);
                spr.fillRect(108, 75, 24, 5, TFT_DARKGREY);
                spr.fillRect(115, 52, 10, 10, TFT_BLACK);

                spr.setTextColor(TFT_DARKGREY);
                spr.loadFont(ArialRoundedMTBold24);
                spr.setTextDatum(TC_DATUM);
                spr.drawString("oil", 120, 85);
            }

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
            // spr_arr.pushSprite(0, 0);
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
            float sx = 90 * cos(sdeg * 0.0174532925) + 120;
            float sy = 90 * sin(sdeg * 0.0174532925) + 120;
            // log_e("x %f; y %f", sx, sy);

            float sx1 = 15 * cos((sdeg - 20) * 0.0174532925) + 120;
            float sy1 = 15 * sin((sdeg - 20) * 0.0174532925) + 120;

            float sx2 = 15 * cos((sdeg + 20) * 0.0174532925) + 120;
            float sy2 = 15 * sin((sdeg + 20) * 0.0174532925) + 120;
            // рисование стрелки
            spr.fillTriangle(sx, sy, sx1, sy1, 120, 120, TFT_RED);
            spr.fillTriangle(sx, sy, sx2, sy2, 120, 120, TFT_RED);
            // spr.drawLine(120, 120, sx, sy, TFT_WHITE);

            // spr.fillCircle(sx, sy, 5, TFT_RED);

            spr.fillCircle(120, 120, 10, TFT_DARKGREY);

            // // текущие показания
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

        // Serial.print(millis() - dt);
        // Serial.println("ms");
        // Получение информации о свободной памяти
        //   size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        //   Serial.print("Свободная память (байты): ");
        //   Serial.println(free_heap);

        // Получение подробной информации о куче
        //   multi_heap_info_t info;
        //   heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
        //   Serial.print("Минимальная свободная память (байты): ");
        //   Serial.println(info.minimum_free_bytes);
        //   Serial.print("Максимальная свободная память (байты): ");
        //   Serial.println(info.largest_free_block);
        //   Serial.print("Использовано памяти (байты): ");
        //   Serial.println(info.total_allocated_bytes);
    }

    //  экран кнопки включения
    void screen_ButtonOn()
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

    //  экран вывода акселератора и среднего расхода
    void screen_ACC()
    {
        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillScreen(TFT_BLACK);
        pngShow("/screen3.png");
        String str{};

        if (data.state == 1)
        {
            if (data.wheelSpeed > 20)
            {
                if (data.cruise == 0)
                    if (data.distLPG.result > 1000)
                        str = String(data.average_gasconsumption, 1);
                    else
                    {
                        str = "Cruise";
                        if (data.engineLoad < 8)
                            color = TFT_RED;
                        else if (data.engineLoad < 27)
                            color = TFT_GREEN;
                        else if (data.engineLoad < 88)
                            color = TFT_ORANGE;
                        else
                            color = TFT_RED;
                    }
            }
            else
            {
                color = TFT_DARKGREY;
                screen_ACC();
            }
        }
        else
        {
            color = TFT_RED;
            str = "Disel";
        }

        if (str == "Disel")
            color = TFT_DARKGREY;
        colorCircle(color);

        // шкала средний расход
        if (data.average_gasconsumption)
        {
            int ll = constrain(map(data.average_gasconsumption, 0, 25, 2, 150), 0, 150);
            if (str == "Disel")
                ll = 0;
            spr.fillRoundRect(44, 168, ll, 6, 2, TFT_SKYBLUE);
        }

        // нижняя строка - запас хода
        spr.loadFont(ArialRoundedMTBold40);
        int x = 120;

        spr.setTextColor(TFT_DARKGREY);
        if (str != "Disel")
            str = String(data.average_gasconsumption, 1);

        spr.setTextDatum(TC_DATUM);
        spr.drawString(str, x, Strok_DOWN_Y);
        spr.unloadFont();

        // верхняя строка - ACC
        spr.loadFont(ArialRoundedMTBold45);
        String s = String(data.ACC);
        
        if (data.ACC = 255)
            s = "---";
      
        spr.setTextColor(color);
        if (str == "Cruise")
            spr.drawString(str, x, Strok_UP_Y);
        else
            spr.drawString(s, x, Strok_UP_Y);
        spr.pushSprite(0, 0, TFT_TRANSPARENT);
        spr.unloadFont();
        spr.deleteSprite();
    }

    // экран вывода параметров
    // void screen_paramEVO(float tred, float presturbo, float presred, int prescng, uint8_t const alarm[], float injection_time, float CNG, String id)
    void screen_paramEVO()
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

            // вывод флагов ошибок EVO
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

    //  экран вывода уровня газа и оставшегося пробега
    void screen_CNGLevel()
    {
        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillScreen(TFT_BLACK);
        pngShow("/screen1.png");

        String str{};
        if (data.state != 1)
        {
            str = "Disel";
            color = TFT_DARKGREY;
        }

        colorCircle(color);

        // полукруглый индикатор уровня
        auto v = CircularScale(9, 200, 60, 120, 110, 85, constrain(data.cngLevel / 2, 0, 100));
        for (auto coord : v)
            if (coord.fill)
            {
                spr.drawSmoothCircle(coord.x, coord.y, 6, TFT_DARKGREY, TFT_BLACK);
                if (constrain(data.cngLevel / 2, 0, 100 > 30))
                    spr.fillCircle(coord.x, coord.y, 6, TFT_DARKGREEN);
                else
                    spr.fillCircle(coord.x, coord.y, 6, TFT_RED);
            }
            else
                spr.drawSmoothCircle(coord.x, coord.y, 6, TFT_DARKGREY, TFT_BLACK);

        // шкала запас хода
        int ll = constrain(map(data.distLPG.gas_mileage, 0, 1000, 0, 150), 0, 150);
        if (str == "Disel")
            ll = 0;
        spr.fillRoundRect(44, 170, ll, 6, 2, TFT_SKYBLUE);

        // нижняя строка
        // int font = 4;
        spr.loadFont(ArialRoundedMTBold40);
        int x = 120;
        spr.setTextColor(TFT_DARKGREY);
        if (str != "Disel")
            str = String(data.distLPG.gas_mileage, 0);

        spr.setTextDatum(TC_DATUM);
        spr.drawString(str, x, Strok_DOWN_Y);
        spr.unloadFont();

        // верхняя строка
        // String s = String(constrain(data.cngLevel / 2, 0, 100));
        String s = String(constrain(map(data.cngLevel, 0, 200, 0, data.tankVolume), 0, data.tankVolume));
        spr.setTextColor(TFT_LIGHTGREY);
        spr.loadFont(ArialRoundedMTBold45);
        spr.drawString(s, x, Strok_UP_Y);
        spr.pushSprite(0, 0);
        spr.unloadFont();
        spr.deleteSprite();
    }

    //  основной информационный экран
    void screen_CNG()
    {
        int x = 84;
        int y = Strok_DOWN_Y;
        // int color = set_color(data.levelEconomicalDriving);
        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillScreen(TFT_BLACK);
        pngShow("/screen4.png");
        String str{};
        if (data.state == 0)
            str = "Disel";

        scale_vertical(data.cngLevel / 2);       // шкала уровень газа
        scale_horizont(data.cngIstValue / 10.0); // шкала средний расход

        if (true_can())
        {
            int cngLevel = constrain(data.cngLevel / 2, 0, 100);

            // средняя строка
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
                res = "";
            spr.loadFont(ArialRoundedMTBold24);
            spr.drawString(res, x, y);
            spr.unloadFont();

            // нижняя строка
            x = 84;
            y = 170;
            spr.loadFont(ArialRoundedMTBold40);
            spr.setTextColor(TFT_DARKGREY);
            if (str == "Disel")
                res = str;
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

            // верхняя строка
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

    //  экран потери связи с газовым ЭБУ
    void screen_EVOLost()
    {
        spr.createSprite(TFT_WIDTH, TFT_HEIGHT);
        spr.fillScreen(TFT_BLACK);
        colorCircle(TFT_RED);
        int font = 4;
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

    void colorCircle(int color)
    {
        for (int i = 0; i < 4; i++)
            spr.drawCircle(120, 120, 120 - i, color);
    }

    void scale_vertical(int x, int y, int l, int h, bool vertical, int vol)
    {
        // символы F E
        spr.setTextColor(TFT_DARKGREY);
        spr.drawRightString("F", x, y, 4);
        spr.drawRightString("E", x, y + h, 4);

        if (vertical)
        { // штрихи белые
            for (int i = 0; i < 9; i++)
            {
                if (i == 4)
                    spr.fillRoundRect(x + 10, y + 10 + 8 * i, l / 2, 3, 1, TFT_WHITE);
                else
                    spr.fillRoundRect(36, 64 + 8 * i, 10, 2, 1, TFT_WHITE);
            }

            int higttY = constrain(map(vol, 5, 200, 192, 48), 48, 192);
            int lengt = constrain(map(vol, 5, 200, 1, 143), 1, 143);
            int color = vol < 30 ? TFT_RED : TFT_DARKGREEN;

            //  шкала
            spr.fillRoundRect(48, higttY, 12, lengt, 2, color);

            // штрихи черные
            for (int i = 0; i < 9; i++)
            {
                spr.fillRoundRect(48, 64 + 8 * i, 10, 2, 1, TFT_BLACK);
            }
        }
    }

    void scale_vertical(int vol)
    {
        int higttY = constrain(map(vol, 5, 100, 192, 48), 48, 192);
        int lengt = constrain(map(vol, 5, 100, 1, 143), 1, 143);
        int color = vol < 30 ? TFT_RED : TFT_DARKGREEN;

        //  шкала
        spr.fillRoundRect(52, 49, 12, 142, 2, getColor(50, 50, 50));
        spr.fillRoundRect(52, higttY, 12, lengt, 2, color);

        // // штрихи черные
        for (int i = 0; i < 9; i++)
            spr.fillRoundRect(52, 63 + 14 * i, 13, 2, 0, TFT_BLACK);
    }

    void scale_horizont(int vol)
    {
        int ll = constrain(map(vol, 0, 25, 0, 143), 0, 143);
        // spr.fillRoundRect(82, 107, 143, 12, 2, TFT_DARKGREY);
        spr.fillRoundRect(82, 107, 143, 12, 2, getColor(50, 50, 50));
        spr.fillRoundRect(82, 107, ll, 12, 2, TFT_SKYBLUE);

        // // штрихи черные
        for (int i = 0; i < 9; i++)
            spr.fillRoundRect(97 + i * 14, 106, 2, 13, 0, TFT_BLACK);
    }

    uint16_t getColor(uint8_t red, uint8_t green, uint8_t blue)
    {
        red >>= 3;
        green >>= 2;
        blue >>= 3;
        return (red << 11) | (green << 5) | blue;
    }

    // проверка, что данные из кан шины валидны (не равны дефолтному значению 0)
    bool true_can()
    {
        return !(data.state == 0 && data.cngWaterTemperature == 0 && data.cngTurboPressure == 0 && data.cngRailPressure == 0 && data.cngLevel == 0 && data.cngInjectionTime == 0 && data.cngIstValue == 0);
    }

public:
    TFT_240_240(fs::LittleFSFS &fs) : filesystem(fs)
    {
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, LOW);
        spr.begin();
        spr.setTextSize(1);
        firstOn();
        if (!filesystem.exists("/NotoSansBold15.vlw"))
        {
            while (1)
                yield();
        }
        if (!filesystem.exists("/NotoSansBold36.vlw"))
        {
            while (1)
                yield();
        }
    }

    int getScreenNowShow()
    {
        return screenNowShow;
    }

    void pngShow(String str)
    {
        int16_t rc = png.open(str.c_str(), pngOpen, pngClose, pngRead, pngSeek, pngDraw);
        rc = png.decode(NULL, 1);
        png.close();
    }

    void backLight(boolean vol)
    {
        if (vol)
        {
            if (screenNowShow != screen::EVOLost)
                for (int i = 0; i < MAX_BRIGHT; i++)
                {
                    analogWrite(TFT_BL, i);
                    delay(1);
                    yield();
                }
            else
                analogWrite(TFT_BL, 20);
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

    void setData(Can_Data dat)
    {
        data = dat;
    }

    void setScreenNumber(int screenNumber)
    {
        if (flag_change_screen)
            return;
        screenNowShow = screen(screenNumber);
    }

    // обновление изображения на дисплее
    void update()
    {
        if (flag_change_screen)
            return;

        //  плавное затемнение перед сменой экрана
        if (screenNowShow != screenNowShowOld)
        {
            screenNowShowOld = screenNowShow;
            flag_change_screen = true;
            backLight(FALSE);
        }

        switch (screenNowShow)
        {
        case screen::ButtonOn:
            screen_ButtonOn();
            break;
        case screen::CNG:
            screen_CNG();
            break;
        case screen::paramEVO:
            screen_paramEVO();
            break;
        case screen::Acc:
            screen_ACC();
            break;
        case screen::LSLevel:
            screen_LLS();
            break;
        case screen::EVOLost:
            screen_EVOLost();
            break;
        default:
            break;
        }
        if (flag_change_screen)
        {
            backLight(TRUE);
            flag_change_screen = false;
        }
        color = set_color(data.levelEconomicalDriving);
    }

    struct Scale
    {
        int x_, y_, l_, h_, vol_;
        int vertical_ = true;
        String label_beg_ = "";
        String label_end_ = "";

    public:
        Scale(int x, int y, int l, int h, bool vertical) : x_(x), y_(y), l_(l), h_(h), vertical_(vertical) {};
        void setLabel(String beg, String end)
        {
            label_beg_ = beg;
            label_end_ = end;
        }

        void setVolume(int vol)
        {
            vol_ = vol;
        }
        void show()
        {
            // символы F E
            spr.setTextColor(TFT_DARKGREY);
            spr.drawRightString("F", x_, y_, 4);
            spr.drawRightString("E", x_, y_ + h_, 4);

            if (vertical_)
            { // штрихи белые
                for (int i = 0; i < 9; i++)
                {
                    if (i == 4)
                        spr.fillRoundRect(x_ + 10, y_ + 10 + 8 * i, l_ / 2, 3, 1, TFT_WHITE);
                    else
                        spr.fillRoundRect(36, 64 + 8 * i, 10, 2, 1, TFT_WHITE);
                }

                int higttY = constrain(map(vol_, 5, 200, 192, 48), 48, 192);
                int lengt = constrain(map(vol_, 5, 200, 1, 143), 1, 143);
                int color = vol_ < 30 ? TFT_RED : TFT_GREEN;

                //  шкала
                spr.fillRoundRect(48, higttY, 12, lengt, 2, color);

                // штрихи черные
                for (int i = 0; i < 9; i++)
                {
                    spr.fillRoundRect(48, 64 + 8 * i, 10, 2, 1, TFT_BLACK);
                }
            }
        }
    };
};