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

#define CAN_RX GPIO_NUM_20
#define CAN_TX GPIO_NUM_21

CanFrame rxFrame;
twai_filter_config_t f_config;

#define FORMAT_LITTLEFS_IF_FAILED false

#define I2C_SDA GPIO_NUM_4
#define I2C_SCL GPIO_NUM_5
#define TP_INT GPIO_NUM_0
#define TP_RST GPIO_NUM_1

#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define DEBUG(x) Serial.println(x)
#else
#define DEBUG(x)
#endif

// Preferences flash;
TFT_240_240 *maindisplay;
CST816D touch(I2C_SDA, I2C_SCL, TP_RST, TP_INT);
uint8_t screenNumber = 0;             //  счетчик экранов
uint8_t gesture;
bool touchUp = false, touchDown = false, touchOn = false;
uint16_t touchX, touchY;

GyverPortal ui(&LittleFS); 

const char *ssid = "DisplayEVO";
const char *password = "00000001";

const char *loginOTA = "ItalGaz";
const char *passwordOTA = "001";

const String VER = "Ver - 2.0 Date - " + String(__DATE__) + "\r";
const String MAP_ACC_RPM_NAME = "Map_ACC-RPM.csv";
const String COPY_MAP_ACC_RPM_NAME = "Copy_Map_ACC-RPM.csv";
const String ERROR_EVO_NEW = "Error_EVO_new.csv";

const float K = 0.1;

const int TIME_SEND_CAN = 400;
const int PERIOD_UPDATE_TFT = 100;
const int PERIOD_UPDATE_UI = 300;

const int COLUM = 34;
const int LINE = 26;

Can_Data data_can{};
uint8_t map_ACC_RPM[LINE][COLUM]{};

unsigned long tim = 0;

// int level = 0;

// uint32_t distance_begin = 0;
// double distance = 0;

int counter_lost_can_vehicle = 0;
int counter_lost_can_EVO = 0;

int canSpeed = 0;                   //  скорость кан-шины
int vehicleType = 0;                //  тип автомобиля

// чтение даннных из кан шины
// void MCP2515_ISR(MCP_CAN &CAN);

// настройка can фильтров
void set_mask_filtr_can_29();
// void set_mask_filtr_can2_29();

// парсинг PGN
void analise_can_id(CanFrame &data);

// void analise_can_id_vehicle(MCP_CAN &CAN, Can_Date &data_can);

// анализатор данных
int calculator(Can_Data &date);

// парсинг карты из файла сsv
bool parse_csv(uint8_t (&)[LINE][COLUM], const String &name);

void send_can(void *pvParameters);
void uiTick(void *pvParameters);
void update_TFT(void *pvParameters);

void wifiInit();

bool compare(File &f1, File &f2);

void buildPage1();
void actionPage1();
String getErrorString(uint8_t err[]);

void twai_generate_filter(uint16_t min, uint16_t max, twai_filter_config_t* filter);
