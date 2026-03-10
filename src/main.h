#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <SPI.h>
#include "TFT_240_240.h"
#include <PNGdec.h>
#include <freertos/task.h>
#include <FreeRTOSConfig.h>
#include <WiFi.h>
#include "CST816D.h"
#include "can_settings.h"

#include <GyverPortal.h>
#include <ESP32-TWAI-CAN.hpp>

// #define WITH_LLS    //  дисплей с выводом уровня топлива
// #define EVO_LOST


#define CAN_RX GPIO_NUM_20
#define CAN_TX GPIO_NUM_21

CanFrame rxFrame;

#define FORMAT_LITTLEFS_IF_FAILED false

#define I2C_SDA GPIO_NUM_4
#define I2C_SCL GPIO_NUM_5
#define TP_INT GPIO_NUM_0
#define TP_RST GPIO_NUM_1

TFT_240_240 *maindisplay;
CST816D touch(I2C_SDA, I2C_SCL, TP_RST, TP_INT);
int8_t screenNumber = 0;             //  счетчик экранов
uint8_t gesture;
uint16_t touchX, touchY;

GyverPortal ui(&LittleFS); 

const char *ssid = "DisplayEVO";
const char *password = "00000001";

const char *loginOTA = "ItalGaz";
const char *passwordOTA = "001";

const String VER = "Ver - 2.1 Date - " + String(__DATE__) + "\r";
const String MAP_ACC_RPM_NAME = "Map_ACC-RPM.csv";
const String COPY_MAP_ACC_RPM_NAME = "Copy_Map_ACC-RPM.csv";
const String ERROR_EVO_NEW = "Error_EVO_new.csv";

const float K = 0.02;

const int TIME_READ_CAN = 100;
const int PERIOD_UPDATE_TFT = 100;
const int PERIOD_UPDATE_UI = 300;
const int TIME_LOST_CAN = 20;
const int PAUSE_TOUCH_ON = 100;
bool flag_touch = false;

#ifdef WITH_LLS
const int NUMBER_SCREEN = 3;    //  количество экранов (3  - с выводом уровня топлива)
#endif

#ifndef WITH_LLS
const int NUMBER_SCREEN = 2;    //  количество экранов (2  - без вывода уровня топлива)
#endif

const int COLUM = 34;
const int LINE = 26;

Can_Data data_can{};
CanFrame canFrame{0}; // структура посылки в кан шину
uint8_t map_ACC_RPM[LINE][COLUM]{};

unsigned long tim = 0;
unsigned long time_touch = 0;

int counter_lost_can_vehicle = 0;
int counter_lost_can_EVO = 0;
bool can_ok = true; // флаг наличия данных по кан
bool gas_on = true;
int canSpeed = 0;                   //  скорость кан-шины
int vehicleType = 0;                //  тип автомобиля
float levelEconomicalDriving = 0;

// парсинг PGN
void analise_can_id(CanFrame &data);

// вычисление уровня оценки экономичности вождения
int calculation_economical_driving(Can_Data &date);

// вычисление среднего расхода газа в поездке
int calculation_average_gasconsumption(Can_Data &date);

// вычисление оставшегося пробега на газе
int calculation_gas_mileage(Can_Data &date);

// парсинг карты из файла сsv
bool parse_csv(uint8_t (&)[LINE][COLUM], const String &name);
void uiTick(void *pvParameters);
void update_TFT(void *pvParameters);
void send_CAN(void *pvParameters);
void watch_dog_CAN(void *pvParameters);
void wifiInit();
bool compare(File &f1, File &f2);
void buildPage();
void actionPage();
String getErrorString(uint8_t err[]);
int lls_tarring(int data);
