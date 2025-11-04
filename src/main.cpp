#include "main.h"
#include <map>

void setup(void)
{

  Serial.begin(115200);

  if (!FileSys.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }
  maindisplay = new TFT_240_240(FileSys);

  touch.begin();
  wifiInit();

  if (!parse_csv(map_ACC_RPM, MAP_ACC_RPM_NAME))
    parse_csv(map_ACC_RPM, COPY_MAP_ACC_RPM_NAME);

  ESP32Can.twaiGenerateFilter(0x1CFFFFFF, 0x18000000, true);
  ESP32Can.begin(ESP32Can.convertSpeed(data_can.canSpeed), CAN_TX, CAN_RX, 10, 10);

  xTaskCreatePinnedToCore(
      update_TFT,        /* Обновление */
      "Task_update_TFT", /* Название задачи */
      4096,              /* Размер стека задачи */
      NULL,              /* Параметр задачи */
      2,                 /* Приоритет задачи */
      NULL,              /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);                /* Ядро для выполнения задачи (1) */

  xTaskCreatePinnedToCore(
      uiTick,        /* Обновление  */
      "Task_uiTick", /* Название задачи */
      4096,          /* Размер стека задачи */
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

  canFrame.identifier = PGN_SEND_ON_EVO; // идентификатор посылки в кан шину: включение блока ево
  canFrame.extd = 1;
  canFrame.data_length_code = 8;

  // подключаем конструктор и запускаем
  ui.uploadAuto(0);   // выключить автозагрузку
  ui.deleteAuto(0);   // выключить автоудаление
  ui.downloadAuto(0); // выключить автоскачивание
  ui.renameAuto(0);   // выключить автопереименование
  ui.attachBuild(buildPage);
  ui.attach(actionPage);
  ui.start();
  ui.enableOTA(loginOTA, passwordOTA);
}

void loop()
{
  ui.tick();
  maindisplay->setData(data_can);

  if (ESP32Can.readFrame(rxFrame, 100))
  {
    can_ok = TRUE;
    analise_can_id(rxFrame);
  }

  if (ESP32Can.busErrCounter() > 94)
  {
    log_e("Error: %d", ESP32Can.busErrCounter());
    ESP32Can.end();
  }

  if (counter_lost_can_EVO > TIME_LOST_CAN)
  {
    can_ok = FALSE;
  }

  if (millis() > time_touch)
  {
    if (touch.getTouch(&touchX, &touchY, &gesture))
    {
      if (flag_touch && gesture != GESTURE::None)
      {
        // log_e("Gesture %0x %0x %0x", touchX, touchY, gesture);
        flag_touch = FALSE;
        if (gesture == GESTURE::SlideDown)
        {
          screenNumber < NUMBER_SCREEN ? screenNumber++ : screenNumber = 0;
          time_touch = millis() + PAUSE_TOUCH_ON;
        }
        else if (gesture == GESTURE::SlideUp)
        {
          screenNumber > 0 ? screenNumber-- : screenNumber = NUMBER_SCREEN;
          time_touch = millis() + PAUSE_TOUCH_ON;
        }
        else if (gesture == GESTURE::LongPress)
        {
          if (maindisplay->getScreenNowShow() == TFT_240_240::ButtonOn)
            if ((touchX > 100 && touchX < 160) && (touchY > 100 && touchY < 150))
            {
              gas_on = !gas_on;
              time_touch = millis() + 20 * PAUSE_TOUCH_ON;
              if (can_ok && millis() > 10000)
              {
                for (int i = 0; i < 7; i++)
                  canFrame.data[i] = 0;
                
                canFrame.data[0] = gas_on;
                canFrame.identifier = PGN_SEND_ON_EVO; // идентификатор посылки в кан шину: включение газового блока EVO PLUS NEW
                ESP32Can.writeFrame(canFrame, 100);
              }
            }
        }
      }
    }
    else
      flag_touch = TRUE;
  }
  //   индикация отсутствия данных от газового блока EVO PLUS NEW
  if (can_ok == FALSE)
  {
    maindisplay->setScreenNumber(TFT_240_240::screen::EVOLost);
    data_can.distLPG.begin = data_can.distance;
  }
  else
    maindisplay->setScreenNumber(screenNumber);
}

void analise_can_id(CanFrame &frame)
{
  // log_e("Frame received %03X: %03X", frame.identifier, frame.data);

  unsigned long PGN = frame.identifier;
  // PGN >>= 8;
  // PGN &= ~(0xff0000);
  switch (PGN)
  {
  case PGN1:
    data_can.state = frame.data[0];
    gas_on = frame.data[0];
    data_can.cngDieselReduction = frame.data[1];
    data_can.ACC = frame.data[4]; // нажатие педали акселератора 0-100%
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
    data_can.cngTotalFuelUsed = (frame.data[4] | frame.data[5] << 8 | frame.data[6] << 16 | frame.data[7] << 24);
    counter_lost_can_EVO = 0;
    break;
  case PGN5:
    data_can.wheelSpeed = (frame.data[1] | frame.data[2] << 8) / 256; // км/час
    data_can.cruise = (frame.data[6] >> 5);
    counter_lost_can_vehicle = 0;
    break;
  case PGN6:
    if (data_can.vehicleType == "FMS")
      data_can.distance = (frame.data[0] | frame.data[1] << 8 | frame.data[2] << 16 | frame.data[3] << 24); // м
    counter_lost_can_vehicle = 0;
    break;
  case PGN7:
    data_can.oilFuelRate = (frame.data[0] | frame.data[1] << 8);
    counter_lost_can_vehicle = 0;
    break;
  case PGN8:
    data_can.engineLoad = (frame.data[2]) - 125;
    data_can.rpm = (frame.data[3] | frame.data[4] << 8) / 8; //  об/мин
    counter_lost_can_vehicle = 0;
    break;
  case PGN9:
    if (data_can.vehicleType == "SHAKMAN")
      data_can.distance = (frame.data[0] | frame.data[1] << 8 | frame.data[2] << 16 | frame.data[3] << 24); // м
    counter_lost_can_vehicle = 0;
    break;
  case PGN10:
    data_can.airTemper = (frame.data[2]) - 40;                                        // C
    data_can.exhaustGasTemper = (frame.data[5] | frame.data[6] << 8) * 0.03125 - 273; // C
    counter_lost_can_vehicle = 0;
    break;
  case PGN11:
    data_can.engineTemper = (frame.data[0]) - 40;
    data_can.fuelTemper = (frame.data[1]) - 40;
    data_can.oilTemper = (frame.data[2] | frame.data[3] << 8) * 0.03125 - 273;
    counter_lost_can_vehicle = 0;
    break;
  case PGN12:
    data_can.vehicleWeight = (frame.data[1] | frame.data[2] << 8);
    counter_lost_can_vehicle = 0;
    break;
  case PGN13:
    data_can.lls = lls_tarring(frame.data[4] | frame.data[5] << 8);
    data_can.full_tank = frame.data[7] * 10;
    break;
  default:
    break;
  }
}

int calculation_economical_driving(Can_Data &data)
{

  int map_acc = constrain(data.ACC / 3, 0, COLUM);
  int map_rpm = constrain((data.rpm - 500) / 100, 0, LINE);

  if (data.rpm < 500)
    map_rpm = 0;

  levelEconomicalDriving = constrain((1.0 - K) * (levelEconomicalDriving) + K * (map_ACC_RPM[map_rpm][map_acc]), 0.0, 100.0);
  // DEBUG("ACC: " + String(data.ACC) + "| RPM: " + String(data.rpm) + " Level: " + levelEconomicalDriving);

  return levelEconomicalDriving;
}

int calculation_average_gasconsumption(Can_Data &data)
{
  int res = 0;
  res = data.cngTripFuel * 100.0 / data.distLPG.result;
  return res;
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

  String patch = "/" + String(name);

  if (!FileSys.exists(patch))
  {
    File file = FileSys.open(patch, FILE_READ);
    log_e("Map %s not found", String(name));
    return false;
  }

  File file = FileSys.open(patch, FILE_READ);
  if (!file)
  {
    log_e("Failed to open map %s", String(name));
    return false;
  }

  std::vector<String> v;
  while (file.available())
    v.push_back(file.readStringUntil('\n'));

  file.close();

  if (v.empty())
  {
    return false;
  }

  for (auto st : v)
  {
    String d = "TypeCan";
    if (st.startsWith(d)) // строка co значением типа кан-шины
    {
      int end = st.indexOf(';', d.length() + 1);
      data_can.vehicleType = st.substring(d.length() + 1, end);
    }

    d = "SpeedCan";
    if (st.startsWith(d)) // строка co значением типа кан-шины
    {
      int end = st.indexOf(';', d.length() + 1);
      data_can.canSpeed = st.substring(d.length() + 1, end).toInt();
    }

    d = "TankVolume";
    if (st.startsWith(d)) // строка co значением объема газовых баллонов
    {
      int end = st.indexOf(';', d.length() + 1);
      data_can.tankVolume = st.substring(d.length() + 1, end).toInt();
    }
  }

  while (!v.front().startsWith("500")) // рабочая часть карты должна начинаться с подстроки 500
  {
    if (v.size() > 0)
    {
      v.erase(v.begin());
    }
    else
    {
      return false;
    }
  }

  int i = 0;
  int j = 0;
  for (String s : v)
  {
    String d = "";
    for (auto ch : s)
      if (ch == ';')
      {
        if (j)
        {
          map[i][j - 1] = d.toInt();
        }
        j++;
        d = "";
      }
      else
      {
        d += ch;
      }
    i++;
    j = 0;
  }

  file = FileSys.open(patch, FILE_READ);
  File fileCopy = FileSys.open("/" + COPY_MAP_ACC_RPM_NAME, FILE_READ);

  if (!compare(file, fileCopy))
  {
    fileCopy.close();
    fileCopy = FileSys.open("/" + COPY_MAP_ACC_RPM_NAME, FILE_WRITE);
    // fileCopy = file;
    while (file.available())
    {
      auto message = file.readString();
      if (!fileCopy.print(message))
        log_e("%s\t- ошибка записи", COPY_MAP_ACC_RPM_NAME);
    }
  }
  file.close();
  fileCopy.close();
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
    if (data_can.state)
    {
      data_can.levelEconomicalDriving = calculation_economical_driving(data_can);
      if ((data_can.distLPG.begin == 0 && data_can.distance != 0) || data_can.rpm < 600)
      {
        data_can.distLPG.begin = data_can.distance;
        data_can.average_gasconsumption = 0.0;
      }
      data_can.distLPG.result = (data_can.distance - data_can.distLPG.begin); // вычисление пробега в поездке (после включения зажигания)
    }
    else
      data_can.distLPG.begin = data_can.distance;

    if (data_can.distLPG.result > 1000)
    {
      data_can.average_gasconsumption = (1 - 5 * K) * data_can.average_gasconsumption + 5 * K * calculation_average_gasconsumption(data_can);
      data_can.distLPG.gas_mileage = (1 - K) * (data_can.distLPG.gas_mileage) + K * (calculation_gas_mileage(data_can));
    }
    if (data_can.average_gasconsumption > 25.5)
      data_can.average_gasconsumption = 25.5;
    if (data_can.distLPG.gas_mileage > 2550.0)
      data_can.distLPG.gas_mileage = 2550.0;
    vTaskDelay(pdMS_TO_TICKS(PERIOD_UPDATE_UI));
  }
  vTaskDelete(NULL);
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
    if (can_ok && millis() > 10000)
    {
      canFrame.identifier = PGN_SEND_DATA; // идентификатор посылки в кан шину: данные для мониторинга
      for (int i = 0; i < 7; i++)
        canFrame.data[i] = 0;
      
      canFrame.data[0] = data_can.levelEconomicalDriving;
      canFrame.data[1] = data_can.average_gasconsumption * 10;
      canFrame.data[2] = data_can.distLPG.gas_mileage / 10;
      canFrame.data[3] = data_can.tankVolume / 10;
      ESP32Can.writeFrame(canFrame, 50);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  vTaskDelete(NULL);
}

void watch_dog_CAN(void *pvParameters)
{
  for (;;)
  {
    counter_lost_can_vehicle++;
    counter_lost_can_EVO++;
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  vTaskDelete(NULL);
}

void wifiInit()
{
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password, 1, 0, 2);
}

// конструктор страницы
void buildPage()
{
  GP.BUILD_BEGIN(800);
  GP.THEME(GP_DARK);
  GP.GRID_RESPONSIVE(500);
  GP.UPDATE("style,canSpeed,engineLoad,vehicleType,tankVolume,distance,rpm,oilFuelRate,wheelSpeed,vehicleWeight,airTemper,engineTemper,oilTemper,fuelTemper,exhaustGasTemper,state,acc,cngInjectionTime,cngIstValue,cngRailPressure,cngTurboPressure,cngLevel,cngRailPressure,cngWaterTemperature,cngRailTemperature,cngTripFuel,cngTotalFuelUsed,cngDieselReduction,error,busErrCounter");
  GP.TITLE("Дисплей EVO | " + VER, "");
  GP.NAV_TABS("Телеметрия,Настройка");
  GP.NAV_BLOCK_BEGIN();
  M_BOX(GP.LABEL("Экономия дизельного топлива, %", "", GP_GRAY_B, 0, 1); GP.LABEL("-", "style", GP_GRAY_B, 0, 1););
  M_GRID(
      M_BLOCK_TAB(
          "АВТОМОБИЛЬ", "400", GP_GRAY_B,
          M_BOX(GP.LABEL("Пробег, км", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "distance"););
          M_BOX(GP.LABEL("Обороты RPM, об/мин", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "rpm"););
          M_BOX(GP.LABEL("Мг. расход ДТ, л/100 км", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "oilFuelRate"););
          M_BOX(GP.LABEL("Скорость, км/ч", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "wheelSpeed"););
          M_BOX(GP.LABEL("Нагрузка двигатель, %", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "engineLoad"););
          M_BOX(GP.LABEL("Вес, т", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "vehicleWeight"););
          M_BOX(GP.LABEL("Т воздух, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "airTemper"););
          M_BOX(GP.LABEL("Т двигатель, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "engineTemper"););
          M_BOX(GP.LABEL("Т масло, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "oilTemper"););
          M_BOX(GP.LABEL("T топливо, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "fuelTemper"););
          M_BOX(GP.LABEL("Т выхлоп, °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "exhaustGasTemper");););
      M_BLOCK_TAB(
          "EVO NEW", "400", GP_ORANGE_B,
          M_BOX(GP.LABEL("Режим", "", GP_GRAY_B, 0, 1); GP.LABEL("-", "state", GP_GRAY_B, 0, 1););
          M_BOX(GP.LABEL("Педаль, %", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "acc"););
          M_BOX(GP.LABEL("Газ, мс", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngInjectionTime"););
          M_BOX(GP.LABEL("Газ, кг/ч", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngIstValue"););
          M_BOX(GP.LABEL("Карта ДТ", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngDieselReduction"););
          M_BOX(GP.LABEL("Турбо, бар", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngTurboPressure"););
          M_BOX(GP.LABEL("Уровень, бар", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngLevel"););
          M_BOX(GP.LABEL("Редуктор, бар", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngRailPressure"););
          M_BOX(GP.LABEL("T ред °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngWaterTemperature"););
          M_BOX(GP.LABEL("T газ °C", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngRailTemperature"););
          M_BOX(GP.LABEL("Расход газ, кг", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngTripFuel"););
          M_BOX(GP.LABEL("Расход итого, кг", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "cngTotalFuelUsed"););
          M_BOX(GP.LABEL("Счетчик кан ошибок", "", GP_GRAY_B, 0, 0); GP.LABEL("-", "busErrCounter"););
          M_BOX(GP.LABEL("Флаги EVO", "", GP_GRAY, 0, 1););
          M_BOX(GP.LABEL("-", "error", GP_RED_B, 0, 0););););
  GP.NAV_BLOCK_END();
  GP.NAV_BLOCK_BEGIN();
  M_GRID(
      M_BLOCK_TAB("",
                  // "Настройки", "400", GP_GRAY_B,
                  M_BOX(GP.LABEL("Скорость can шины, кБ"); GP.LABEL(String(data_can.canSpeed), "canSpeed"););
                  M_BOX(GP.LABEL("Тип данных в can"); GP.LABEL(data_can.vehicleType, "vehicleType"););
                  M_BOX(GP.LABEL("Объем газовых баллонов, л"); GP.LABEL(String(data_can.tankVolume), "tankVolume"););
                  GP.FILE_UPLOAD("file_upl", "Загрузить файл настроек", "", GP_ORANGE_B); // кнопка загрузки
                  GP.FILE_MANAGER(&LittleFS);););                                         // файловый менеджер
  GP.NAV_BLOCK_END();
  GP.FORM_END();
  GP.BUILD_END();
}

void actionPage()
{
  if (ui.update())
  {
    ui.updateInt("canSpeed", data_can.canSpeed);
    ui.updateString("vehicleType", data_can.vehicleType);
    ui.updateInt("tankVolume", data_can.tankVolume);
    ui.updateInt("busErrCounter", ESP32Can.busErrCounter());

    if (counter_lost_can_EVO < TIME_LOST_CAN * 2)
    {
      String str = data_can.state ? "Газ" : "Дт";
      auto t = data_can.levelEconomicalDriving;
      ui.updateInt("style", t);
      ui.updateString("state", str);
      ui.updateInt("acc", data_can.ACC);
      ui.updateInt("cngLevel", data_can.cngLevel);
      ui.updateInt("cngRailTemperature", data_can.cngRailTemperature - 40);
      ui.updateInt("cngWaterTemperature", data_can.cngWaterTemperature);
      ui.updateInt("cngDieselReduction", data_can.cngDieselReduction);
      ui.updateFloat("cngTurboPressure", data_can.cngTurboPressure / 100.0, 1);
      ui.updateFloat("cngRailPressure", data_can.cngRailPressure / 50.0, 1);
      ui.updateFloat("cngTripFuel", data_can.cngTripFuel / 1000.0, 1);
      ui.updateFloat("cngTotalFuelUsed", data_can.cngTotalFuelUsed / 2.0, 1);
      ui.updateFloat("cngInjectionTime", data_can.cngInjectionTime / 10.0, 1);
      ui.updateFloat("cngIstValue", data_can.cngIstValue / 10.0, 1);
      str = getErrorString(data_can.errorEVO);
      ui.updateString("error", str);
    }
    else
    {
      String str = "no can EVO";
      ui.updateInt("style", 0);
      ui.updateString("state", str);
      ui.updateInt("acc", 0);
      ui.updateInt("cngLevel", 0);
      ui.updateInt("cngRailTemperature", 0);
      ui.updateInt("cngWaterTemperature", 0);
      ui.updateInt("cngDieselReduction", 0);
      ui.updateInt("cngTurboPressure", 0);
      ui.updateInt("cngRailPressure", 0);
      ui.updateInt("cngTripFuel", 0);
      ui.updateInt("cngTotalFuelUsed", 0);
      ui.updateInt("cngInjectionTime", 0);
      ui.updateInt("cngIstValue", 0);
      ui.updateString("error", str);
    }

    if (can_ok)
    {
      float d = data_can.distLPG.result / 1000.0;
      ui.updateFloat("distance", d);
      ui.updateInt("rpm", data_can.rpm);
      ui.updateFloat("oilFuelRate", data_can.oilFuelRate / 20.0, 1);
      ui.updateInt("wheelSpeed", data_can.wheelSpeed);
      ui.updateInt("engineLoad", data_can.engineLoad);
      ui.updateFloat("vehicleWeight", data_can.vehicleWeight / 2000.0, 1);
      ui.updateInt("airTemper", data_can.airTemper);
      ui.updateInt("engineTemper", data_can.engineTemper);
      ui.updateInt("oilTemper", data_can.oilTemper);
      ui.updateInt("fuelTemper", data_can.fuelTemper);
      ui.updateInt("exhaustGasTemper", data_can.exhaustGasTemper);
    }
    else
    {
      ui.updateInt("distance", 0);
      ui.updateInt("rpm", 0);
      ui.updateInt("oilFuelRate", 0);
      ui.updateInt("wheelSpeed", 0);
      ui.updateInt("engineLoad", 0);
      ui.updateInt("vehicleWeight", 0);
      ui.updateInt("airTemper", 0);
      ui.updateInt("engineTemper", 0);
      ui.updateInt("oilTemper", 0);
      ui.updateInt("fuelTemper", 0);
      ui.updateInt("exhaustGasTemper", 0);
    }
  }

  if (ui.upload())
  {
    ui.saveFile(LittleFS.open('/' + ui.fileName(), "w")); // сохранение в корень по имени файла
  }

  // ===== ФАЙЛОВЫЙ МЕНЕДЖЕР =====
  // обработчик скачивания файлов (для открытия в браузере)
  if (ui.download())
    ui.sendFile(LittleFS.open(ui.uri(), "r"));

  // // обработчик удаления файлов
  // if (ui.deleteFile())
  // {
  //   LittleFS.remove(ui.deletePath());
  //   Serial.println(ui.deletePath());
  // }

  // // обработчик переименования файлов
  // if (ui.renameFile())
  //   LittleFS.rename(ui.renamePath(), ui.renamePathTo());
}

String getErrorString(uint8_t err[])
{
  String error{};
  File file = FileSys.open("/" + ERROR_EVO_NEW, FILE_READ);
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
  std::map<int, int> table_taring{{1, 0}, {107, 200}, {344, 400}, {572, 600}, {800, 800}, 
                                  {988, 1000}, {1193, 1200}, {1400, 1400}, {1606, 1600}, 
                                  {1803, 1800}, {2006, 2000}, {2206, 2200}, {2407, 2400}, 
                                  {2608, 2600}, {2804, 2800}, {3005, 3000}, {3202, 3200}, 
                                  {3423, 3400}, {3600, 3600}, {3812, 3800}, {3863, 3850}};
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