#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <GyverPortal.h>

struct WebPageCfg
{
  String key;
  String title;
  bool enabled = false;
  int order = 0;
  bool systemPage = false;
};

struct WebPortalData
{
  int canSpeed = 0;
  String vehicleType;
  int tankVolume = 0;
  int busErrCounter = 0;

  bool canOk = false;
  bool evoOk = false;
  uint32_t lostEvo = 0;
  bool state = false;

  int levelEconomicalDriving = 0;
  int ACC = 0;
  int cngLevel = 0;
  int cngRailTemperature = 0;
  int cngWaterTemperature = 0;
  int cngDieselReduction = 0;
  float cngTurboPressure = 0;
  float cngRailPressure = 0;
  float cngTripFuel = 0;
  float cngTotalFuelUsed = 0;
  float cngInjectionTime = 0;
  float cngIstValue = 0;
  String error;

  float distance = 0;
  int rpm = 0;
  float oilFuelRate = 0;
  float wheelSpeed = 0;
  int engineLoad = 0;
  float vehicleWeight = 0;
  int airTemper = 0;
  int engineTemper = 0;
  int oilTemper = 0;
  int fuelTemper = 0;
  int exhaustGasTemper = 0;

  static constexpr uint8_t WEB_PAGE_COUNT = 6;
  WebPageCfg pageCfg[WEB_PAGE_COUNT];
};

class WebPortal
{
public:
  using DataProvider = WebPortalData (*)();
  using ApplyConfigCallback = bool (*)(const String& fileName);
  using SavePagesCallback = void (*)(const WebPageCfg* pages, uint8_t count);

  WebPortal(fs::FS& fs,
            const char* ssid,
            const char* password,
            const char* otaLogin,
            const char* otaPassword,
            DataProvider dataProvider,
            ApplyConfigCallback applyConfig,
            SavePagesCallback savePages,
            const String& versionText);

  void begin();
  void tick();

private:
  static WebPortal* _self;

  fs::FS& _fs;
  GyverPortal _ui;

  const char* _ssid;
  const char* _password;
  const char* _otaLogin;
  const char* _otaPassword;
  String _versionText;

  DataProvider _dataProvider;
  ApplyConfigCallback _applyConfig;
  SavePagesCallback _savePages;

  void beginWifi();
  void buildPage();
  void actionPage();

  void updateDisplayPageConfig(int index, bool enabled, int order);
  void normalizeDisplayPageOrder();

  static void buildPageThunk();
  static void actionPageThunk();
};