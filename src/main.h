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
#include "WebPortal.h"
#include <SolenoidHealth.h>

#include <ESP32-TWAI-CAN.hpp>
#include <DisplayConfig.h>

Valve valves[4] = {{SolenoidHealth::OK, 0},  
                  {SolenoidHealth::OK, 0},
                  {SolenoidHealth::OK, 0},
                  {SolenoidHealth::OK, 0}}; // массив для хранения состояния клапанов баллонов

#define CAN_RX GPIO_NUM_20
#define CAN_TX GPIO_NUM_21

CanFrame rxFrame;
#define FORMAT_LITTLEFS_IF_FAILED false

#define I2C_SDA GPIO_NUM_4
#define I2C_SCL GPIO_NUM_5
#define TP_INT GPIO_NUM_0
#define TP_RST GPIO_NUM_1

TFT_240_240 *maindisplay = nullptr;
CST816D touch(I2C_SDA, I2C_SCL, TP_RST, TP_INT);
uint8_t gesture;
uint16_t touchX, touchY;

const char *ssid = "DisplayEVO";
const char *password = "00000001";

const char *loginOTA = "ItalGaz";
const char *passwordOTA = "001";

const String VER = "Ver - 3.1 Date - " + String(__DATE__) + "\r";

const float K = 0.2;

const int TIME_READ_CAN = 100;
const int PERIOD_UPDATE_TFT = 300;
const int PERIOD_UPDATE_UI = 300;
const int TIME_LOST_CAN = 20;
const int PAUSE_TOUCH_ON = 200;

const float KG_TO_M3 = 0.85f;

const int COLUM = 34;                           //  количество столбцов в карте ACC\RPM
const int LINE = 26;                            //  количество строк в карте ACC\RPM

Can_Data data_can{};
CanFrame canFrame{0};                           // структура посылки в кан шину
uint8_t map_ACC_RPM[LINE][COLUM]{};

const String ERROR_EVO_NEW{"Error_EVO_new.csv"};
std::vector<String> error_evo{};

unsigned long tim = 0;
unsigned long time_touch = 0;

int counter_lost_can_vehicle = 0;
int counter_lost_can_EVO = 0;
bool can_ok = true; // флаг наличия данных по кан
bool gas_on = true; // флаг работы на газе, устанавливается на основе данных с КАН шины, используется для начала отсчета расхода газа в поездке и расчета уровня экономичного вождения
float levelEconomicalDriving = 0; // уровень экономичного вождения, рассчитывается на основе карты ACC\RPM и данных с КАН шины


bool startGas = false; // флаг начала отсчета расхода газа во время движения на газе

// парсинг PGN
void analise_can_id(CanFrame &data);

// вычисление уровня оценки экономичности вождения
int calculation_economical_driving(Can_Data &date);

// вычисление среднего расхода газа в поездке
float calculation_average_gasconsumption(Can_Data &date);

// вычисление оставшегося пробега на газе
int calculation_gas_mileage(Can_Data &date);

// загрузка карты ACC\RPM из внешнего файла в массив map_ACC_RPM
bool loadAccRpmFile(uint8_t (&)[LINE][COLUM], const String &name);

void uiTick(void *pvParameters);
void send_CAN(void *pvParameters);
void watch_dog_CAN(void *pvParameters);
void taskDisplay(void *pvParameters);
// String (uint8_t err[]);
int lls_tarring(int data);

bool loadDisplayConfig();
bool saveDisplayConfig();

void errorStringtoVec(std::vector<String> &errror_evo);
String updateStringError(uint8_t err[], std::vector<String> &error_evo);

static portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;

extern DisplayConfig g_displayConfig;
QueueHandle_t g_displayQueue = nullptr;

enum class DisplayCmdType : uint8_t
{
    UpdateData,
    ShowPage,
    NextPage,
    PrevPage,
    // ForceRefresh
};

struct DisplayMessage
{
    DisplayCmdType type;
    Can_Data data;
    int page;
};

static inline Can_Data snapshotDataCan()
{
  Can_Data copy;
  portENTER_CRITICAL(&dataMux);
  copy = data_can;
  portEXIT_CRITICAL(&dataMux);
  return copy;
}

static inline bool snapshotCanOk()
{
  bool v;
  portENTER_CRITICAL(&dataMux);
  v = can_ok;
  portEXIT_CRITICAL(&dataMux);
  return v;
}

static void sendCanFrame(uint32_t id, const uint8_t *payload, uint8_t len = 8, uint32_t timeout = 100)
{
  CanFrame tx{};
  tx.identifier = id;
  tx.extd = 1;
  tx.data_length_code = (len > 8) ? 8 : len;

  memset(tx.data, 0, sizeof(tx.data));
  if (payload != nullptr)
    memcpy(tx.data, payload, tx.data_length_code);

  ESP32Can.writeFrame(tx, timeout);
}

static WebPortalData makeWebPortalData()
{
  WebPortalData d;

  Can_Data local;
  bool localCanOk;
  uint32_t localLostEvo;

  portENTER_CRITICAL(&dataMux);
  local = data_can;
  localCanOk = can_ok;
  localLostEvo = counter_lost_can_EVO;
  portEXIT_CRITICAL(&dataMux);

  d.canSpeed = local.canSpeed;
  d.vehicleType = local.vehicleType;
  d.tankVolume = local.tankVolume;
  d.busErrCounter = ESP32Can.busErrCounter();

  d.canOk = localCanOk;
  d.lostEvo = localLostEvo;
  d.evoOk = (localLostEvo < TIME_LOST_CAN * 2);
  d.state = local.state;

  d.levelEconomicalDriving = local.levelEconomicalDriving;
  d.ACC = local.ACC;
  d.cngLevel = local.cngLevel;
  d.cngRailTemperature = local.cngRailTemperature;
  d.cngWaterTemperature = local.cngWaterTemperature;
  d.cngDieselReduction = local.cngDieselReduction;
  d.cngTurboPressure = local.cngTurboPressure;
  d.cngRailPressure = local.cngRailPressure;
  d.cngTripFuel = local.cngTripFuel;
  d.cngTotalFuelUsed = local.cngTotalFuelUsed;
  d.cngInjectionTime = local.cngInjectionTime;
  d.cngIstValue = local.cngIstValue;
  d.error = updateStringError(local.errorEVO, error_evo);

  // d.distance = local.distLPG.result / 1000.0f;
  d.distance = local.distance;
  d.rpm = local.rpm;
  d.oilFuelRate = local.oilFuelRate;
  d.wheelSpeed = local.wheelSpeed;
  d.engineLoad = local.engineLoad;
  d.vehicleWeight = local.vehicleWeight;
  d.airTemper = local.airTemper;
  d.engineTemper = local.engineTemper;
  d.oilTemper = local.oilTemper;
  d.fuelTemper = local.fuelTemper;
  d.exhaustGasTemper = local.exhaustGasTemper;

  // Копирование конфигурации страниц в web-данные
    for (int i = 0; i < WebPortalData::WEB_PAGE_COUNT; ++i)
    {
        d.pageCfg[i].key = g_displayConfig.pages[i].key;
        d.pageCfg[i].title = g_displayConfig.pages[i].title;
        d.pageCfg[i].enabled = g_displayConfig.pages[i].enabled;
        d.pageCfg[i].order = g_displayConfig.pages[i].order;
        d.pageCfg[i].systemPage = g_displayConfig.pages[i].systemPage;
    }

  return d;
}

static bool applyUploadedConfig(const String& fileName)
{
  // if (fileName == MAP_ACC_RPM_NAME || fileName == COPY_MAP_ACC_RPM_NAME)
  if (fileName == MAP_ACC_RPM_NAME)
  {
    bool applied = loadAccRpmFile(map_ACC_RPM, fileName);
    if (applied)
    {
      int localCanSpeed;
      portENTER_CRITICAL(&dataMux);
      localCanSpeed = data_can.canSpeed;
      portEXIT_CRITICAL(&dataMux);

      ESP32Can.begin(ESP32Can.convertSpeed(localCanSpeed), CAN_TX, CAN_RX, 10, 10);
    }
    return applied;
  }

  return false;
}

static void normalizeDisplayPageOrder()
{
    uint8_t nextOrder = 0;

    for (uint8_t desired = 0; desired < DISPLAY_PAGE_COUNT; ++desired)
    {
        for (uint8_t i = 0; i < DISPLAY_PAGE_COUNT; ++i)
        {
            auto& p = g_displayConfig.pages[i];
            if (!p.systemPage && p.enabled && p.order == desired)
            {
                p.order = nextOrder++;
            }
        }
    }

    for (uint8_t i = 0; i < DISPLAY_PAGE_COUNT; ++i)
    {
        auto& p = g_displayConfig.pages[i];
        if (!p.systemPage && p.enabled && p.order >= DISPLAY_PAGE_COUNT)
        {
            p.order = nextOrder++;
        }
    }
}

static void applyPagesFromWeb(const WebPageCfg* pages, uint8_t count)
{
    for (uint8_t i = 0; i < count && i < DISPLAY_PAGE_COUNT; ++i)
    {
        if (g_displayConfig.pages[i].systemPage)
            continue;

        g_displayConfig.pages[i].enabled = pages[i].enabled;
        g_displayConfig.pages[i].order   = pages[i].order;
    }

    normalizeDisplayPageOrder();

    if (maindisplay)
        maindisplay->setDisplayConfig(g_displayConfig);

    saveDisplayConfig();
}

static void applyGeneralConfigFromWeb(const WebGeneralConfig& cfg)
{
    data_can.canSpeed = cfg.canSpeed;
    data_can.vehicleType = cfg.vehicleType;
    data_can.tankVolume = cfg.tankVolume;

    saveDisplayConfig();

    ESP32Can.begin(ESP32Can.convertSpeed(data_can.canSpeed), CAN_TX, CAN_RX, 10, 10);

    if (maindisplay)
        maindisplay->setDisplayConfig(g_displayConfig);
}

WebPortal webPortal(
    LittleFS,
    ssid,
    password,
    loginOTA,
    passwordOTA,
    makeWebPortalData,
    applyUploadedConfig,
    applyPagesFromWeb,
    applyGeneralConfigFromWeb,
    VER);