#include "main.h"
#include "WebPortal.h"
#include <map>
#include <ArduinoJson.h>

#include "DisplayConfig.h"

DisplayConfig g_displayConfig = {
    .pages = {
        {DisplayPageId::CNG, "cng", "CNG", true, 0, false},
        {DisplayPageId::ParamEVO, "paramEvo", "Параметры EVO", true, 1, false},
        {DisplayPageId::ButtonOn, "button", "Кнопка газ/дт", true, 2, false},
        {DisplayPageId::LSLevel, "lls", "Уровень топлива", true, 3, false},
        {DisplayPageId::ValveTank, "valve", "Соленоиды", true, 4, false},
        {DisplayPageId::EVOLost, "evoLost", "EVO Lost", true, 255, true},
    }};

void setup(void)
{

  Serial.begin(115200);

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    Serial.println("LittleFS mount failed");
  }

  maindisplay = new TFT_240_240();

  touch.begin();

  if (!parse_csv(map_ACC_RPM, MAP_ACC_RPM_NAME))
    parse_csv(map_ACC_RPM, COPY_MAP_ACC_RPM_NAME);

  ESP32Can.twaiGenerateFilter(0x1CFFFFFF, 0x18000000, true);
  ESP32Can.begin(ESP32Can.convertSpeed(data_can.canSpeed), CAN_TX, CAN_RX, 10, 10);

  xTaskCreatePinnedToCore(
      update_TFT,        /* Обновление */
      "Task_update_TFT", /* Название задачи */
      8192,              /* Размер стека задачи */
      NULL,              /* Параметр задачи */
      2,                 /* Приоритет задачи */
      NULL,              /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);                /* Ядро для выполнения задачи (1) */

  xTaskCreatePinnedToCore(
      uiTick,        /* Обновление  */
      "Task_uiTick", /* Название задачи */
      8192,          /* Размер стека задачи */
      NULL,          /* Параметр задачи */
      1,             /* Приоритет задачи */
      NULL,          /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);            /* Ядро для выполнения задачи (1) */

  xTaskCreatePinnedToCore(
      send_CAN,       /* Обновление  */
      "Task_sendCAN", /* Название задачи */
      4096,           /* Размер стека задачи */
      NULL,           /* Параметр задачи */
      0,              /* Приоритет задачи */
      NULL,           /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);             /* Ядро для выполнения задачи (1) */

  xTaskCreatePinnedToCore(
      watch_dog_CAN,        /* Обновление  */
      "Task_watch_dog_CAN", /* Название задачи */
      2048,                 /* Размер стека задачи */
      NULL,                 /* Параметр задачи */
      10,                   /* Приоритет задачи */
      NULL,                 /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);                   /* Ядро для выполнения задачи (1) */

  loadDisplayConfig();
  maindisplay->setDisplayConfig(g_displayConfig);
  delay(1000);
  maindisplay->showPage(DisplayPageId::ButtonOn);
  webPortal.begin();
}

void loop()
{
  webPortal.tick();

  Can_Data dataSnap = snapshotDataCan();
  maindisplay->setData(dataSnap, valves);

  if (ESP32Can.readFrame(rxFrame, 100))
  {
    portENTER_CRITICAL(&dataMux);
    can_ok = true;
    portEXIT_CRITICAL(&dataMux);

    analise_can_id(rxFrame);
  }

  if (millis() > time_touch)
  {
    if (touch.getTouch(&touchX, &touchY, &gesture))
    {
      if (flag_touch && gesture != GESTURE::None)
      {
        flag_touch = false;

        if (gesture == GESTURE::SlideDown)
        {
          // portENTER_CRITICAL(&dataMux);
          maindisplay->nextUserPage();
          // portEXIT_CRITICAL(&dataMux);
          time_touch = millis() + PAUSE_TOUCH_ON;
        }
        else if (gesture == GESTURE::SlideUp)
        {
          // portENTER_CRITICAL(&dataMux);
          maindisplay->prevUserPage();
          // portEXIT_CRITICAL(&dataMux);
          time_touch = millis() + PAUSE_TOUCH_ON;
        }
        else if (gesture == GESTURE::LongPress)
        {
          if (maindisplay->getScreenNowShow() == static_cast<int>(DisplayPageId::ButtonOn))
          {
            if ((touchX > 100 && touchX < 160) && (touchY > 100 && touchY < 150))
            {
              bool localGasOn;
              bool localCanOk;

              // portENTER_CRITICAL(&dataMux);
              gas_on = !gas_on;
              localGasOn = gas_on;
              localCanOk = can_ok;
              // portEXIT_CRITICAL(&dataMux);

              time_touch = millis() + 20 * PAUSE_TOUCH_ON;

              if (localCanOk && millis() > 10000)
              {
                uint8_t payload[8] = {0};
                payload[0] = localGasOn ? 1 : 0;
                sendCanFrame(PGN_SEND_ON_EVO, payload, 8, 100);
              }
            }
          }
        }
      }
    }
    else
    {
      flag_touch = true;
    }
  }

  bool localCanOk;
  uint8_t localScreen;

  portENTER_CRITICAL(&dataMux);
  localCanOk = can_ok;
  // localScreen = screenNumber;
  portEXIT_CRITICAL(&dataMux);

  if (!localCanOk)
  {
    // maindisplay->setScreenNumber(static_cast<int>(DisplayPageId::EVOLost));

    portENTER_CRITICAL(&dataMux);
    data_can.distLPG.begin = data_can.distance;
    portEXIT_CRITICAL(&dataMux);
  }
  else
  {
    // maindisplay->setScreenNumber(localScreen);
  }
}

void analise_can_id(CanFrame &frame)
{
  unsigned long PGN = frame.identifier;

  portENTER_CRITICAL(&dataMux);

  switch (PGN)
  {
  case PGN1:
    data_can.state = frame.data[0];
    gas_on = frame.data[0];
    data_can.cngDieselReduction = frame.data[1];
    data_can.ACC = frame.data[4];
    data_can.cngInjectionTime = frame.data[2];
    data_can.cngTurboPressure = frame.data[3];
    data_can.cngRailPressure = frame.data[5];
    data_can.cngRailTemperature = frame.data[6];
    counter_lost_can_EVO = 0;
    break;

  case PGN2:
    data_can.cngLevel = frame.data[6];
    data_can.cngWaterTemperature = frame.data[7];
    counter_lost_can_EVO = 0;
    break;

  case PGN3:
    data_can.errorEVO[0] = frame.data[0];
    data_can.errorEVO[1] = frame.data[1];
    data_can.errorEVO[2] = frame.data[2];
    data_can.errorEVO[3] = frame.data[3];
    data_can.errorEVO[4] = frame.data[4];
    data_can.errorEVO[5] = frame.data[5];
    data_can.verID = frame.data[6];
    data_can.verFMlow = frame.data[7];
    data_can.cruise = frame.data[3] & B00000001;
    counter_lost_can_EVO = 0;
    break;

  case PGN4:
    data_can.cngIstValue = frame.data[3];
    data_can.cngTripFuel = frame.data[0] | frame.data[1] << 8 | frame.data[2] << 16;
    data_can.cngTotalFuelUsed = frame.data[4] | frame.data[5] << 8 | frame.data[6] << 16 | frame.data[7] << 24;
    counter_lost_can_EVO = 0;
    break;

  case PGN5:
    data_can.wheelSpeed = (frame.data[1] | frame.data[2] << 8) / 256.0f;
    // log_e("Скорость %d", data_can.wheelSpeed);
    counter_lost_can_vehicle = 0;
    break;

  case PGN6:
    data_can.distance = (frame.data[0] | frame.data[1] << 8 | frame.data[2] << 16 | frame.data[3] << 24) * 5 / 1000.0f;

    // data_can.engineLoad = frame.data[0];
    counter_lost_can_vehicle = 0;

    // log_e("Пробег %d", data_can.distance);

    break;

  case PGN7:
    data_can.oilFuelRate = (frame.data[0] | frame.data[1] << 8) * 5 / 200.0f;

    // data_can.airTemper = frame.data[0] - 40;
    counter_lost_can_vehicle = 0;
    break;

  case PGN8:
    data_can.rpm = (frame.data[3] | frame.data[4] << 8) / 8.0;
    counter_lost_can_vehicle = 0;
    break;

  case PGN9:
    data_can.exhaustGasTemper = frame.data[0] | frame.data[1] << 8;
    counter_lost_can_vehicle = 0;
    break;

  case PGN10:
    counter_lost_can_vehicle = 0;
    break;

  case PGN11:
    data_can.engineTemper = frame.data[0] - 40;
    data_can.fuelTemper = frame.data[1] - 40;
    data_can.oilTemper = (frame.data[2] | frame.data[3] << 8) * 0.03125 - 273;
    counter_lost_can_vehicle = 0;
    break;

  case PGN12:
    data_can.vehicleWeight = (frame.data[1] | frame.data[2] << 8) / 2000.0f;
    counter_lost_can_vehicle = 0;
    break;

  case PGN13:
    data_can.lls = lls_tarring(frame.data[4] | frame.data[5] << 8);
    data_can.full_tank = frame.data[7] * 10;
    break;

  case PGN14:
    valves[0].updateState(static_cast<SolenoidHealth>(frame.data[6] & B00000111), frame.data[2]);
    valves[1].updateState(static_cast<SolenoidHealth>((frame.data[6] >> 4) & B00000111), frame.data[3]);
    valves[2].updateState(static_cast<SolenoidHealth>((frame.data[7]) & B00000111), frame.data[4]);
    valves[3].updateState(static_cast<SolenoidHealth>((frame.data[7] >> 4) & B00000111), frame.data[5]);
    break;

  default:
    break;
  }

  portEXIT_CRITICAL(&dataMux);
}

int calculation_economical_driving(Can_Data &data)
{
  int map_acc = constrain(data.ACC / 3, 0, COLUM - 1);
  int map_rpm = constrain((data.rpm - 500) / 100, 0, LINE - 1);

  if (data.rpm < 500)
    map_rpm = 0;

  levelEconomicalDriving = constrain(
      (1.0 - K) * levelEconomicalDriving + K * map_ACC_RPM[map_rpm][map_acc],
      0.0, 100.0);

  return levelEconomicalDriving;
}

int calculation_average_gasconsumption(Can_Data &data)
{
  if (data.distLPG.result == 0)
    return 0;

  return data.cngTripFuel * 100.0 / data.distLPG.result;
}

int calculation_gas_mileage(Can_Data &data)
{
  int res{};
  if (data.average_gasconsumption && data.cngTripFuel)
    res = data.cngLevel * data.tankVolume / (2 * KG_TO_M3 * data.average_gasconsumption);
  else
    res = data.cngLevel * data.tankVolume / (2 * KG_TO_M3 * 20);
  return res;
}

bool parse_csv(uint8_t (&map)[LINE][COLUM], const String &name)
{
  const String path = "/" + name;

  if (!LittleFS.exists(path))
  {
    log_e("Map %s not found", name.c_str());
    return false;
  }

  File file = LittleFS.open(path, FILE_READ);
  if (!file)
  {
    log_e("Failed to open map %s", name.c_str());
    return false;
  }

  std::vector<String> lines;
  while (file.available())
  {
    String s = file.readStringUntil('\n');
    s.trim();
    if (s.length())
      lines.push_back(s);
  }
  file.close();

  if (lines.empty())
    return false;

  String newVehicleType;
  int newCanSpeed = 0;
  int newTankVolume = 0;

  bool hasVehicleType = false;
  bool hasCanSpeed = false;
  bool hasTankVolume = false;

  for (const String &st : lines)
  {
    if (st.startsWith("TypeCan;"))
    {
      int pos = st.indexOf(';');
      if (pos >= 0)
      {
        newVehicleType = st.substring(pos + 1);
        newVehicleType.replace(";", "");
        hasVehicleType = true;
      }
    }
    else if (st.startsWith("SpeedCan;"))
    {
      int pos = st.indexOf(';');
      if (pos >= 0)
      {
        newCanSpeed = st.substring(pos + 1).toInt();
        hasCanSpeed = true;
      }
    }
    else if (st.startsWith("TankVolume;"))
    {
      int pos = st.indexOf(';');
      if (pos >= 0)
      {
        newTankVolume = st.substring(pos + 1).toInt();
        hasTankVolume = true;
      }
    }
  }

  size_t firstMapLine = 0;
  while (firstMapLine < lines.size() && !lines[firstMapLine].startsWith("500"))
    ++firstMapLine;

  if (firstMapLine >= lines.size())
  {
    log_e("Map data section not found in %s", name.c_str());
    return false;
  }

  memset(map, 0, sizeof(map));

  int row = 0;
  for (size_t k = firstMapLine; k < lines.size() && row < LINE; ++k, ++row)
  {
    const String &s = lines[k];
    int col = 0;
    int start = 0;

    while (start <= s.length() && col < (COLUM + 1))
    {
      int sep = s.indexOf(';', start);
      if (sep < 0)
        sep = s.length();

      String token = s.substring(start, sep);
      token.trim();

      if (col > 0)
      {
        int mapCol = col - 1;
        if (mapCol < COLUM)
          map[row][mapCol] = static_cast<uint8_t>(constrain(token.toInt(), 0, 255));
      }

      ++col;
      start = sep + 1;

      if (sep >= s.length())
        break;
    }
  }

  portENTER_CRITICAL(&dataMux);
  if (hasVehicleType)
    data_can.vehicleType = newVehicleType;
  if (hasCanSpeed)
    data_can.canSpeed = newCanSpeed;
  if (hasTankVolume)
    data_can.tankVolume = newTankVolume;
  portEXIT_CRITICAL(&dataMux);

  File src = LittleFS.open(path, FILE_READ);
  if (!src)
    return true;

  File dst = LittleFS.open("/" + String(COPY_MAP_ACC_RPM_NAME), FILE_WRITE);
  if (!dst)
  {
    src.close();
    log_e("Failed to open backup map file");
    return true;
  }

  while (src.available())
    dst.write(src.read());

  src.close();
  dst.close();

  return true;
}

bool compare(File &f1, File &f2)
{
  if (f2.size() != f1.size())
    return false;
  while (f1.available())
  {
    if (f2.available())
    {
      if (f1.read() != f2.read())
        return false;
    }
    else
      return false;
  }
  return true;
}

void uiTick(void *pvParameters)
{
  for (;;)
  {
    Can_Data loc_canDat{};

    portENTER_CRITICAL(&dataMux);
    loc_canDat = data_can;
    portEXIT_CRITICAL(&dataMux);

    if (loc_canDat.state)
    {
      loc_canDat.levelEconomicalDriving = calculation_economical_driving(loc_canDat);

      if ((loc_canDat.distLPG.begin == 0 && loc_canDat.distance != 0) || loc_canDat.rpm < 600)
      {
        loc_canDat.distLPG.begin = loc_canDat.distance;
        loc_canDat.average_gasconsumption = 0.0;
      }

      loc_canDat.distLPG.result = loc_canDat.distance - loc_canDat.distLPG.begin;
    }
    else
    {
      loc_canDat.distLPG.begin = loc_canDat.distance;
    }

    if (loc_canDat.distLPG.result > 1000)
    {
      if (loc_canDat.distLPG.result != 0)
      {
        loc_canDat.average_gasconsumption =
            (1 - 5 * K) * loc_canDat.average_gasconsumption +
            5 * K * calculation_average_gasconsumption(loc_canDat);
      }

      loc_canDat.distLPG.gas_mileage =
          (1 - K) * loc_canDat.distLPG.gas_mileage +
          K * calculation_gas_mileage(loc_canDat);
    }

    if (loc_canDat.average_gasconsumption > 25.5)
      loc_canDat.average_gasconsumption = 25.5;

    if (loc_canDat.distLPG.gas_mileage > 2550.0)
      loc_canDat.distLPG.gas_mileage = 2550.0;

    portENTER_CRITICAL(&dataMux);
    data_can.levelEconomicalDriving = loc_canDat.levelEconomicalDriving;
    data_can.distLPG.begin = loc_canDat.distLPG.begin;
    data_can.distLPG.result = loc_canDat.distLPG.result;
    data_can.average_gasconsumption = loc_canDat.average_gasconsumption;
    data_can.distLPG.gas_mileage = loc_canDat.distLPG.gas_mileage;
    portEXIT_CRITICAL(&dataMux);

    vTaskDelay(pdMS_TO_TICKS(PERIOD_UPDATE_UI));
  }
}

// вывод данных на экран
void update_TFT(void *pvParameters)
{
  for (;;)
  {
    maindisplay->update();
    vTaskDelay(pdMS_TO_TICKS(PERIOD_UPDATE_TFT));
  }
  vTaskDelete(NULL);
}

// отправка сообщения в КАН шину
void send_CAN(void *pvParameters)
{
  for (;;)
  {
    Can_Data loc_canDat{};
    bool localCanOk;

    portENTER_CRITICAL(&dataMux);
    loc_canDat = data_can;
    localCanOk = can_ok;
    portEXIT_CRITICAL(&dataMux);

    if (localCanOk && millis() > 10000)
    {
      uint8_t payload[8] = {0};
      payload[0] = loc_canDat.levelEconomicalDriving;
      payload[1] = static_cast<uint8_t>(loc_canDat.average_gasconsumption * 10);
      payload[2] = static_cast<uint8_t>(loc_canDat.distLPG.gas_mileage / 10);
      payload[3] = static_cast<uint8_t>(loc_canDat.tankVolume / 10);

      sendCanFrame(PGN_SEND_DATA, payload, 8, 50);
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void watch_dog_CAN(void *pvParameters)
{
  for (;;)
  {
    portENTER_CRITICAL(&dataMux);
    counter_lost_can_vehicle++;
    counter_lost_can_EVO++;
    portEXIT_CRITICAL(&dataMux);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

String getErrorString(uint8_t err[])
{
  String error{};
  File file = LittleFS.open("/" + ERROR_EVO_NEW, FILE_READ);
  std::vector<String> str_err{};

  int i = 0;
  while (file.available())
  {
    String s = file.readStringUntil('\n');
    str_err.push_back(s);
  }
  file.close();

  for (int i = 0; i < 6; i++)
  {
    uint8_t errEvo = err[i];
    for (int j = 0; j < 8; j++)
    {
      errEvo & 1 ? error += str_err.at(i * 8 + j).length() ? String(i * 8 + j + 1) + " " + str_err.at(i * 8 + j) + "<br>" : str_err.at(i * 8 + j) : error;
      errEvo = errEvo >> 1;
    }
  }
  return error;
}

int lls_tarring(int data)
{
  std::map<int, int> table_taring{{1, 0}, {107, 200}, {344, 400}, {572, 600}, {800, 800}, {988, 1000}, {1193, 1200}, {1400, 1400}, {1606, 1600}, {1803, 1800}, {2006, 2000}, {2206, 2200}, {2407, 2400}, {2608, 2600}, {2804, 2800}, {3005, 3000}, {3202, 3200}, {3423, 3400}, {3600, 3600}, {3812, 3800}, {3863, 3850}};
  auto iterator = table_taring.begin();

  for (int i = 0; i < table_taring.size(); i++)
  {
    if (data > iterator->first)
      iterator++;
    else
    {
      auto min = iterator;
      iterator++;
      auto max = iterator;
      return map(data, min->first, max->first, min->second, max->second);
    }
  }
  if (data < 4096)
    return 3900;
  else
    return -1;
};

bool loadDisplayConfig()
{
  File configFile = LittleFS.open("/display_config.json", "r");
  if (!configFile)
  {
    Serial.println("loadDisplayConfig: file not found");
    return false;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error)
  {
    Serial.println("loadDisplayConfig: json parse failed");
    return false;
  }

  JsonArray pages = doc["pages"].as<JsonArray>();
  if (pages.isNull())
  {
    Serial.println("loadDisplayConfig: pages array missing");
    return false;
  }

  for (int i = 0; i < DISPLAY_PAGE_COUNT && i < pages.size(); ++i)
  {
    g_displayConfig.pages[i].enabled = pages[i]["enabled"].as<bool>();
    g_displayConfig.pages[i].order = pages[i]["order"].as<uint8_t>();
  }

  Serial.println("loadDisplayConfig: ok");
  return true;
}

bool saveDisplayConfig()
{
  DynamicJsonDocument doc(1024);
  JsonArray pages = doc.createNestedArray("pages");

  for (int i = 0; i < DISPLAY_PAGE_COUNT; ++i)
  {
    JsonObject page = pages.createNestedObject();
    page["enabled"] = g_displayConfig.pages[i].enabled;
    page["order"] = g_displayConfig.pages[i].order;
  }

  File configFile = LittleFS.open("/display_config.json", "w");
  if (!configFile)
  {
    return false;
  }
  serializeJson(doc, configFile);
  configFile.close();
  return true;
}
