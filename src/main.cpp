#include "main.h"
#include "WebPortal.h"
#include <map>
#include <ArduinoJson.h>

#include "DisplayConfig.h"

//#define DEBUG

DisplayConfig g_displayConfig = {
    .pages = {
        {DisplayPageId::CNG, "cng", "CNG", true, 1, false},
        {DisplayPageId::ParamEVO, "paramEvo", "Параметры EVO", true, 0, false},
        {DisplayPageId::ButtonOn, "button", "Кнопка газ/дт", true, 2, false},
        {DisplayPageId::LSLevel, "lls", "Уровень топлива", false, 3, false},
        {DisplayPageId::ValveTank, "valve", "Соленоиды", false, 4, false},
        {DisplayPageId::EVOLost, "evoLost", "EVO Lost", true, 255, true},
    }};

void setup(void)
{
  Serial.begin(115200);

  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    log_e("LittleFS mount failed");
  }
  // Try to mount, if failed, format
  // if(!LittleFS.begin(true)){ // 'true' formats on failure
    // Serial.println("LittleFS Mount Failed, formatting...");
    // If 'true' in begin() isn't enough, add:
    // LittleFS.format();
    // LittleFS.begin();
  // } else {
    // Serial.println("LittleFS Mounted Successfully");
  // }
  
  maindisplay = new TFT_240_240();
  touch.begin();
  

  loadAccRpmFile(map_ACC_RPM, MAP_ACC_RPM_NAME); //  загрузка карты ACC\RPM
  errorStringtoVec(error_evo);                   //  загрузка списка ошибок газового блока EVO из внешнего файла
  loadDisplayConfig();                           //  загрузка настроек дисплея из json файла
  maindisplay->setDisplayConfig(g_displayConfig);

  ESP32Can.twaiGenerateFilter(0x1CFFFFFF, 0x18000000, true);
  ESP32Can.begin(ESP32Can.convertSpeed(data_can.canSpeed), CAN_TX, CAN_RX, 10, 10);
  g_displayQueue = xQueueCreate(8, sizeof(DisplayMessage));
  if (g_displayQueue == nullptr)
  {
    // log_e("display queue create failed");
    while (1)
      yield();
  }

  xTaskCreate(
      uiTick,        /* Обновление  */
      "Task_uiTick", /* Название задачи */
      4096,          /* Размер стека задачи */
      NULL,          /* Параметр задачи */
      1,             /* Приоритет задачи */
      NULL);         /* Идентификатор задачи, чтобы ее можно было отслеживать *);            /* Ядро для выполнения задачи (1) */

  xTaskCreate(
      send_CAN,       /* Обновление  */
      "Task_sendCAN", /* Название задачи */
      4096,           /* Размер стека задачи */
      NULL,           /* Параметр задачи */
      0,              /* Приоритет задачи */
      NULL);           /* Идентификатор задачи, чтобы ее можно было отслеживать */

  xTaskCreate(
      watch_dog_CAN,        /* Обновление  */
      "Task_watch_dog_CAN", /* Название задачи */
      2048,                 /* Размер стека задачи */
      NULL,                 /* Параметр задачи */
      10,                   /* Приоритет задачи */
      NULL);                /* Идентификатор задачи, чтобы ее можно было отслеживать */

  xTaskCreate(
      taskDisplay,
      "taskDisplay",
      4096,
      nullptr,
      3,
      nullptr);

  delay(1000);

  if (g_displayQueue)
  {
    DisplayMessage msg{};
    msg.type = DisplayCmdType::NextPage;
    xQueueSend(g_displayQueue, &msg, 0);
  }
  webPortal.begin();

#ifdef DEBUG
  for (;;)
  {
    yield();
  }
#endif
}

void loop()
{
  webPortal.tick();

  Can_Data dataSnap = snapshotDataCan();

  if (counter_lost_can_vehicle > TIME_LOST_CAN) //  если нет данных по кан более TIME_LOST_CAN секунд вешаем флаг ошибки
  {
    portENTER_CRITICAL(&dataMux);
    can_ok = false;
    portEXIT_CRITICAL(&dataMux);
  }

  if (ESP32Can.readFrame(rxFrame, 100))
  {
    portENTER_CRITICAL(&dataMux);
    can_ok = true;
    portEXIT_CRITICAL(&dataMux);

    analise_can_id(rxFrame);

    if (g_displayQueue)
    {
      DisplayMessage msg{};
      msg.type = DisplayCmdType::UpdateData;
      msg.data = data_can;
      xQueueSend(g_displayQueue, &msg, 0); // только если очередь длиной 1}
    }
  }

  if (millis() > time_touch)
  {
    if (touch.getTouch(&touchX, &touchY, &gesture))
    {
      // log_e("Touch: x=%d, y=%d, gesture=%d", touchX, touchY, gesture); // не удалять вывод жестов, он нужен для настройки координат и распознавания жестов на дисплее
      switch (gesture)
      {
      case GESTURE::SlideDown:
        if (g_displayQueue)
        {
          DisplayMessage msg{};
          msg.type = DisplayCmdType::NextPage;
          xQueueSend(g_displayQueue, &msg, 0);
        }
        time_touch = millis() + PAUSE_TOUCH_ON;
        break;
      case GESTURE::SlideUp:
        if (g_displayQueue)
        {
          DisplayMessage msg{};
          msg.type = DisplayCmdType::PrevPage;
          xQueueSend(g_displayQueue, &msg, 0);
        }
        time_touch = millis() + PAUSE_TOUCH_ON;
        break;
      case GESTURE::LongPress:
        if ((maindisplay->getScreenNowShow() == static_cast<int>(DisplayPageId::ButtonOn)) && ((touchX > 30 && touchX < 210) && (touchY > 30 && touchY < 210)))
        {
          bool localGasOn;
          bool localCanOk;
          portENTER_CRITICAL(&dataMux);
          gas_on = !gas_on;
          localGasOn = gas_on;
          localCanOk = can_ok;
          portEXIT_CRITICAL(&dataMux);
          time_touch = millis() + 20 * PAUSE_TOUCH_ON;
          if (localCanOk && millis() > 10000)
          {
            uint8_t payload[8] = {0};
            payload[0] = localGasOn ? 1 : 0;
            sendCanFrame(PGN_SEND_ON_EVO, payload, 8, 100);
          }
        }
        break;
      default:
        break;
      }
    }
  }
}

/*
    анализ входящего CAN сообщения, обновление данных в структуре data_can
    и состояния клапанов баллонов в массиве valves на основе данных из КАН шины
*/
void analise_can_id(CanFrame &frame)
{
  unsigned long PGN_32 = frame.identifier;
  portENTER_CRITICAL(&dataMux);
  switch (PGN_32)
  {
  case PGN1:
    data_can.state = frame.data[0];
    gas_on = frame.data[0];
    data_can.cngDieselReduction = frame.data[1];
    data_can.ACC = frame.data[4];
    data_can.cngInjectionTime = frame.data[2] / 10.f;
    data_can.cngTurboPressure = frame.data[3] / 100.0f;
    data_can.cngRailPressure = frame.data[5] / 50.f;
    data_can.cngRailTemperature = frame.data[6] - 40;
    data_can.exhaustGasTemper = frame.data[7] * 5;
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
    data_can.cngIstValue = frame.data[3] / 10.0f;
    data_can.cngTripFuel = (frame.data[0] | frame.data[1] << 8 | frame.data[2] << 16) / 1000.0f;
    data_can.cngTotalFuelUsed = (frame.data[4] | frame.data[5] << 8 | frame.data[6] << 16 | frame.data[7] << 24) / 2.0f;
    counter_lost_can_EVO = 0;
    break;

  case PGN13:
    valves[0].updateState(static_cast<SolenoidHealth>(frame.data[6] & B00000111), frame.data[2], frame.data[1] & B00000001);
    valves[1].updateState(static_cast<SolenoidHealth>((frame.data[6] >> 4) & B00000111), frame.data[3], (frame.data[1] & B00000010) >> 1);
    valves[2].updateState(static_cast<SolenoidHealth>((frame.data[7]) & B00000111), frame.data[4], (frame.data[1] & B00000100) >> 2);
    valves[3].updateState(static_cast<SolenoidHealth>((frame.data[7] >> 4) & B00000111), frame.data[5], (frame.data[1] & B00001000) >> 3);
    break;

  case PGN14:
    data_can.full_tank = frame.data[0] * 10;
    data_can.lls = lls_tarring(frame.data[1] | frame.data[2] << 8);
    break;

  default:
    break;
  }

  uint16_t PGN = (uint16_t)(PGN_32 >> 8);
  switch (PGN)
  {
  case PGN5:
    data_can.wheelSpeed = (frame.data[1] | frame.data[2] << 8) / 256.0f;
    counter_lost_can_vehicle = 0;
    break;

  case PGN6:
    data_can.distance = (frame.data[0] | frame.data[1] << 8 | frame.data[2] << 16 | frame.data[3] << 24) / 200.0f;
    counter_lost_can_vehicle = 0;
    break;

  case PGN7:
    data_can.oilFuelRate = (frame.data[0] | frame.data[1] << 8) / 20.0f;
    counter_lost_can_vehicle = 0;
    break;

  case PGN8:
    data_can.rpm = (frame.data[3] | frame.data[4] << 8) / 8.0;
    data_can.engineLoad = frame.data[2] - 125;
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

  default:
    break;
  }
  portEXIT_CRITICAL(&dataMux);
}

/*
    рассчет уровня экономичного вождения на основе карты ACC\RPM и данных с КАН шины,
    сглаживание результата с помощью фильтра скользящего среднего
*/
int calculation_economical_driving(Can_Data &data)
{
  int map_acc = constrain(data.ACC / 3, 0, COLUM - 1);
  int map_rpm = constrain((data.rpm - 500) / 100, 0, LINE - 1);

  if (data.rpm < 500)
    map_rpm = 0;

  levelEconomicalDriving = constrain((1.0 - K) * levelEconomicalDriving + K * map_ACC_RPM[map_rpm][map_acc], 0.0, 100.0);
  return levelEconomicalDriving;
}

//  рассчет среднего расхода газа на основе данных с КАН шины
float calculation_average_gasconsumption(Can_Data &data)
{
  if (data.distLPG.result == 0)
    return 0;
  return data.cngTripFuel * 100.0 / data.distLPG.result;
}

//  рассчет пробега на запасе газа на основе данных с КАН шины
int calculation_gas_mileage(Can_Data &data)
{
  int res{};
  if (data.average_gasconsumption && data.cngTripFuel)
    res = data.cngLevel * data.tankVolume / (2 * KG_TO_M3 * data.average_gasconsumption);
  else
    res = data.cngLevel * data.tankVolume / (2 * KG_TO_M3 * 20);
  return res;
}

// загрузка карты ACC\RPM из внешнего файла в массив map_ACC_RPM
bool loadAccRpmFile(uint8_t (&map)[LINE][COLUM], const String &name)
{
  const String path = "/" + name;

  if (!LittleFS.exists(path))
  {
    // log_e("Map %s not found", name.c_str());
    return false;
  }

  File file = LittleFS.open(path, FILE_READ);
  if (!file)
  {
    // log_e("Failed to open map %s", name.c_str());
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

  size_t firstMapLine = 0;
  while (firstMapLine < lines.size() && !lines[firstMapLine].startsWith("500"))
    ++firstMapLine;

  if (firstMapLine >= lines.size())
  {
    // log_e("Map data section not found in %s", name.c_str());
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

  File src = LittleFS.open(path, FILE_READ);
  if (!src)
    return true;

  src.close();
  return true;
}

/*
    обновление данных для отображения на TFT дисплее,
    расчет уровня экономичного вождения, среднего расхода газа
    и газового пробега на основе данных с КАН шины
*/
void uiTick(void *pvParameters)
{
  for (;;)
  {
    Can_Data loc_canDat{};

    portENTER_CRITICAL(&dataMux);
    loc_canDat = data_can;
    portEXIT_CRITICAL(&dataMux);

    if (gas_on)
    {
      loc_canDat.levelEconomicalDriving = calculation_economical_driving(loc_canDat);

      if (startGas == false)
      {
        loc_canDat.distLPG.begin = loc_canDat.distance;
        loc_canDat.average_gasconsumption = 0.0;
        startGas = true;
      }
      loc_canDat.distLPG.result = loc_canDat.distance - loc_canDat.distLPG.begin;
    }
    else
    {
      loc_canDat.distLPG.begin = loc_canDat.distance;
      startGas = false;
    }

    if (loc_canDat.rpm < 500)
    {
      loc_canDat.distLPG.begin = loc_canDat.distance;
      startGas = false;
    }

    // log_e("Distance: %.2f km, Trip Fuel: %.2f l, Average Gas Consumption: %.2f l/100km, Gas Mileage: %.2f km",
    // loc_canDat.distLPG.result, loc_canDat.cngTripFuel, loc_canDat.average_gasconsumption, loc_canDat.distLPG.gas_mileage);

    if (loc_canDat.distLPG.result > 1.0)
    {
      loc_canDat.average_gasconsumption = calculation_average_gasconsumption(loc_canDat);

      // if (loc_canDat.average_gasconsumption > 25.5)
      // loc_canDat.average_gasconsumption = 25.5;
    }

    loc_canDat.distLPG.gas_mileage = (1 - K) * loc_canDat.distLPG.gas_mileage + K * calculation_gas_mileage(loc_canDat);
    if (loc_canDat.distLPG.gas_mileage > 2550.0)
      loc_canDat.distLPG.gas_mileage = 2550.0;

    portENTER_CRITICAL(&dataMux);
    data_can.levelEconomicalDriving = loc_canDat.levelEconomicalDriving;
    data_can.distLPG.begin = loc_canDat.distLPG.begin;
    data_can.distLPG.result = loc_canDat.distLPG.result;
    data_can.average_gasconsumption = loc_canDat.average_gasconsumption;
    data_can.distLPG.gas_mileage = loc_canDat.distLPG.gas_mileage;
    portEXIT_CRITICAL(&dataMux);

    if (g_displayQueue)
    {
      DisplayMessage msg{};
      msg.type = DisplayCmdType::ShowPage; // обновление данных для отображения на TFT дисплее
      xQueueSend(g_displayQueue, &msg, 0); // только если очередь длиной 1}
    }

    vTaskDelay(pdMS_TO_TICKS(PERIOD_UPDATE_UI));
  }
  vTaskDelete(NULL);
}

// вывод данных на TFT дисплее
void taskDisplay(void *pvParameters)
{
  DisplayMessage msg{};
  Can_Data lastData{};
  int lastPage = 0;
  for (;;)
  {
    if (xQueueReceive(g_displayQueue, &msg, portMAX_DELAY) == pdTRUE)
    {
      switch (msg.type)
      {
      case DisplayCmdType::UpdateData:
        lastData = msg.data;
        maindisplay->setData(lastData, valves);
        break;
      case DisplayCmdType::ShowPage:
        lastPage = msg.page;
        maindisplay->update();
        break;
      case DisplayCmdType::NextPage:
        maindisplay->nextUserPage();
        break;
      case DisplayCmdType::PrevPage:
        maindisplay->prevUserPage();
        break;
      }
    }
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
      payload[1] = static_cast<uint8_t>(loc_canDat.average_gasconsumption * 2);
      payload[2] = static_cast<uint8_t>(loc_canDat.distLPG.gas_mileage / 10);
      payload[3] = static_cast<uint8_t>(loc_canDat.tankVolume / 10);

      sendCanFrame(PGN_SEND_DATA, payload, 8, 100);
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  vTaskDelete(NULL);
}

// контроль потери данных с КАН шины
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
  vTaskDelete(NULL);
}

// тарирование данных уровня топлива в дизельном баке по данным с датчика LLS
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

// загрузка настроек дисплея в json файл
bool loadDisplayConfig()
{
  File configFile = LittleFS.open("/display_config.json", "r");
  if (!configFile)
  {
    log_i("loadDisplayConfig: file not found");
    return false;
  }

  DynamicJsonDocument doc(1536);
  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error)
  {
    log_i("loadDisplayConfig: json parse failed");
    return false;
  }

  if (doc.containsKey("canSpeed"))
    data_can.canSpeed = doc["canSpeed"].as<int>();

  if (doc.containsKey("vehicleType"))
    data_can.vehicleType = doc["vehicleType"].as<String>();

  if (doc.containsKey("tankVolume"))
    data_can.tankVolume = doc["tankVolume"].as<int>();

  JsonArray pages = doc["pages"].as<JsonArray>();
  if (!pages.isNull())
  {
    for (int i = 0; i < DISPLAY_PAGE_COUNT && i < pages.size(); ++i)
    {
      g_displayConfig.pages[i].enabled = pages[i]["enabled"].as<bool>();
      // g_displayConfig.pages[i].order = pages[i]["order"].as<uint8_t>();
    }
  }

  log_i("loadDisplayConfig: ok");
  return true;
}

// сохранение настроек дисплея в json файл
bool saveDisplayConfig()
{
  DynamicJsonDocument doc(1536);

  doc["canSpeed"] = data_can.canSpeed;
  doc["vehicleType"] = data_can.vehicleType;
  doc["tankVolume"] = data_can.tankVolume;

  JsonArray pages = doc.createNestedArray("pages");

  for (int i = 0; i < DISPLAY_PAGE_COUNT; ++i)
  {
    JsonObject page = pages.createNestedObject();
    page["key"] = g_displayConfig.pages[i].key;
    page["enabled"] = g_displayConfig.pages[i].enabled;
    // page["order"] = g_displayConfig.pages[i].order;

    // log_e("key %s : enable %d", g_displayConfig.pages[i].key, g_displayConfig.pages[i].enabled);
  }

  File configFile = LittleFS.open("/display_config.json", "w");
  if (!configFile)
  {
    // log_e("saveDisplayConfig: open failed");
    return false;
  }

  if (serializeJsonPretty(doc, configFile) == 0)
  {
    // log_e("saveDisplayConfig: write failed");
    configFile.close();
    return false;
  }

  configFile.close();
  // log_e("saveDisplayConfig: ok");
  return true;
}

// загрузка списка ошибок газового блока EVO из внешнего файла в вектор строк ошибок газового блока EVO
void errorStringtoVec(std::vector<String> &error_evo)
{
  String error{};
  File file = LittleFS.open("/" + ERROR_EVO_NEW, FILE_READ);
  std::vector<String> str_err{};

  int i = 0;
  while (file.available())
  {
    String s = file.readStringUntil('\n');
    error_evo.push_back(s);
  }
  file.close();
}

// обновление строки с ошибками газового блока EVO на основе данных из КАН шины и списка ошибок из вектора строк ошибок газового блока EVO
String updateStringError(uint8_t err[], std::vector<String> &error_evo)
{
  String error{};

  for (int i = 0; i < 6; i++)
  {
    uint8_t errEvo = err[i];
    for (int j = 0; j < 8; j++)
    {
      errEvo & 1 ? error += error_evo.at(i * 8 + j).length() ? String(i * 8 + j + 1) + " " + error_evo.at(i * 8 + j) + "<br>" : error_evo.at(i * 8 + j) : error;
      errEvo = errEvo >> 1;
    }
  }
  if (error.length() == 0)
    error = "Ошибок нет";
  return error;
}
