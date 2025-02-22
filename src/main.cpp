#include "main.h"

void setup(void)
{
  Serial.begin(115200);

  while (!Serial)
  {
    yield(); // Stay here twiddling thumbs waiting
  }

  if (!FileSys.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    DEBUG("LittleFS initialisation failed!");
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }

  maindisplay = new TFT_240_240;
  touch.begin();
  wifiInit();

  if (!parse_csv(map_ACC_RPM, MAP_ACC_RPM_NAME))
    parse_csv(map_ACC_RPM, COPY_MAP_ACC_RPM_NAME);

  // Set pins
  ESP32Can.setPins(CAN_TX, CAN_RX);

  // You can set custom size for the queues - those are default
  ESP32Can.setRxQueueSize(5);
  ESP32Can.setTxQueueSize(1);
  set_mask_filtr_can_29();

  // .setSpeed() and .begin() functions require to use TwaiSpeed enum,
  // but you can easily convert it from numerical value using .convertSpeed()
  ESP32Can.setSpeed(ESP32Can.convertSpeed(data_can.canSpeed));

  // You can also just use .begin()..
  if (ESP32Can.begin())
    DEBUG("CAN BUS Shield init ok! Speed: " + String(data_can.canSpeed));
  else
    DEBUG("CAN BUS init fail");

  xTaskCreatePinnedToCore(
      update_TFT,        /* Обновление WiFi */
      "Task_update_TFT", /* Название задачи */
      8192,              /* Размер стека задачи */
      NULL,              /* Параметр задачи */
      1,                 /* Приоритет задачи */
      NULL,              /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);                /* Ядро для выполнения задачи (1) */

  // xTaskCreatePinnedToCore(
  //     send_can,        /* Обновление  */
  //     "Task_send_can", /* Название задачи */
  //     2048,            /* Размер стека задачи */
  //     NULL,            /* Параметр задачи */
  //     2,               /* Приоритет задачи */
  //     NULL,            /* Идентификатор задачи, чтобы ее можно было отслеживать */
  //     1);              /* Ядро для выполнения задачи (1) */

  xTaskCreatePinnedToCore(
      uiTick,        /* Обновление  */
      "Task_uiTick", /* Название задачи */
      4096,          /* Размер стека задачи */
      NULL,          /* Параметр задачи */
      2,             /* Приоритет задачи */
      NULL,          /* Идентификатор задачи, чтобы ее можно было отслеживать */
      1);            /* Ядро для выполнения задачи (1) */

  // подключаем конструктор и запускаем
  ui.uploadAuto(0);   // выключить автозагрузку
  ui.deleteAuto(0);   // выключить автоудаление
  ui.downloadAuto(0); // выключить автоскачивание
  ui.renameAuto(0);   // выключить автопереименование
  ui.attachBuild(buildPage1);
  ui.attach(actionPage1);
  ui.start();
  ui.enableOTA(loginOTA, passwordOTA);
}

void loop()
{
  bool FingerNum = touch.getTouch(&touchX, &touchY, &gesture);
  if (FingerNum)
  {
    // Serial.printf("X:%d,Y:%d,gesture:%x\n", touchX, touchY, gesture);
    gesture == 1 ? touchDown = true : touchDown = false;
    gesture == 2 ? touchUp = true : touchUp = false;

    if (maindisplay->getScreenNowShow() == 0)
    {
      if ((touchX > 100 && touchX < 140) && (touchY > 100 && touchY < 10))
        data_can.state ? touchOn = false : touchOn = true;
    }
  }

  // You can set custom timeout, default is 1000
  if (ESP32Can.readFrame(rxFrame, 0))
  {
    analise_can_id(rxFrame);
  }
  maindisplay->updateData(&data_can);
  data_can.levelEconomicalDriving = calculator(data_can);
}

void analise_can_id(CanFrame &frame)
{
  unsigned long PGN = frame.identifier;
  // PGN >>= 8;
  // PGN &= ~(0xff0000);
  switch (PGN)
  {
  case PGN1:
    data_can.state = frame.data[0];
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
    if (data_can.vehicleType == "FMS")
      data_can.distance = (frame.data[0] | frame.data[1] << 8 | frame.data[2] << 16 | frame.data[3] << 24); // м
    break;
  case PGN6:
    data_can.rpm = (frame.data[3] | frame.data[4] << 8) / 8; //  об/мин
    data_can.engineLoad = data_can.engineLoad * (1.0 - K) + K * (frame.data[2] - 125);
    counter_lost_can_vehicle = 0;
    break;
  case PGN7:
    data_can.oilFuelRate = (frame.data[0] | frame.data[1] << 8);
    counter_lost_can_vehicle = 0;
    break;
  case PGN8:
    data_can.engineLoad = (frame.data[0] | frame.data[1] << 8);
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
  default:
    break;
  }
}

int calculator(Can_Data &data)
{
  DEBUG("ACC: " + String(data.ACC) + "| RPM: " + String(data.rpm));

  int map_acc = constrain(data.ACC / 3, 0, COLUM);
  int map_rpm = constrain((data.rpm - 500) / 100, 0, LINE);

  if (data.rpm < 500)
    map_rpm = 0;

  // Serial.printf("Hor: %d | Vert: %d | MAP: %d\n", map_acc, map_rpm, map_ACC_RPM[map_rpm][map_acc]);
  uint8_t level = constrain((1.0 - K) * ((float)level) + K * ((float)map_ACC_RPM[map_rpm][map_acc]), 0, 100);

  return level;
}

void set_mask_filtr_can_29()
{
  twai_generate_filter(*FILTR_ID.begin(), *FILTR_ID.end(), &f_config);
};

bool parse_csv(uint8_t (&map)[LINE][COLUM], const String &name)
{

  String patch = "/" + String(name);

  if (!FileSys.exists(patch))
  {
    File file = FileSys.open(patch, FILE_READ);
    DEBUG("Map " + String(name) + " not found\n");
    return false;
  }

  File file = FileSys.open(patch, FILE_READ);
  if (!file)
  {
    DEBUG("Failed to open map " + String(name) + "\n");
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
        DEBUG(COPY_MAP_ACC_RPM_NAME + "\t- ошибка записи");
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
    ui.tick();
    vTaskDelay(pdMS_TO_TICKS(PERIOD_UPDATE_UI));
  }
  vTaskDelete(NULL);
}

// вывод данных на экран
void update_TFT(void *pvParameters)
{
  for (;;)
  {
    // Serial.printf("Counter lost can EVO: %d\n", counter_lost_can_EVO);
    // Serial.printf("TouchDown: %d\n", touchDown);
    // Serial.printf("TouchUp: %d\n", touchUp);

    if (touchDown)
    {
      screenNumber++;
      touchDown = 0;
    }
    if (touchUp)
    {
      screenNumber--;
      touchUp = 0;
    }

    if (screenNumber > 3)
      screenNumber = 0;
    if (screenNumber < 0)
      screenNumber = 3;

    // Serial.printf("ScreenNumber: %d\n", screenNumber);

    //   индикация отсутствия данных от газового блока
    if (counter_lost_can_EVO++ > 200)
    {
      maindisplay->updateScreen(4);
      data_can.distLPG.begin = data_can.distance;
    }
    else
    {
      if (data_can.state == 1)
      {
        if (data_can.distLPG.begin == 0 && data_can.distance != 0)
          data_can.distLPG.begin = data_can.distance;
        data_can.distLPG.result = (data_can.distance - data_can.distLPG.begin); // вычисление пробега в поездке (после включения зажигания)
      }
      else
        data_can.distLPG.begin = data_can.distance;
      maindisplay->updateScreen(screenNumber);
    }
    vTaskDelay(pdMS_TO_TICKS(PERIOD_UPDATE_TFT));
  }
  vTaskDelete(NULL);
}

// отправка данных в кан-шину
void send_can(void *pvParameters)
{
  for (;;)
  {
    CanFrame obdFrame{};
    obdFrame.identifier = 0x18FFFABB; // Default OBD2 address;
    obdFrame.extd = 1;
    obdFrame.data_length_code = 8;

    obdFrame.data[0] = 0xAA;
    obdFrame.data[1] = 0xAA;
    obdFrame.data[2] = 0xAA;
    obdFrame.data[3] = 0xAA; // Best to use 0xAA (0b10101010) instead of 0
    obdFrame.data[4] = 0xAA; // CAN works better this way as it needs
    obdFrame.data[5] = 0xAA; // to avoid bit-stuffing
    obdFrame.data[6] = 0xAA;
    obdFrame.data[7] = touchOn;
    // // Accepts both pointers and references
    ESP32Can.writeFrame(obdFrame); // timeout defaults to 1 ms
    vTaskDelay(pdMS_TO_TICKS(TIME_SEND_CAN));
  }
  vTaskDelete(NULL);
}

void wifiInit()
{
  WiFi.mode(WIFI_AP);
  DEBUG("\nWiFi start");
  WiFi.softAP(ssid, password, 1, 0, 2);
  DEBUG(WiFi.softAPIP()); // по умолч. 192.168.4.1
}

// конструктор страницы
void buildPage1()
{
  GP.BUILD_BEGIN(800);
  GP.THEME(GP_DARK);
  GP.GRID_RESPONSIVE(500);
  GP.UPDATE("style,canSpeed,engineLoad,vehicleType,tankVolume,distance,rpm,oilFuelRate,wheelSpeed,vehicleWeight,airTemper,engineTemper,oilTemper,fuelTemper,exhaustGasTemper,state,acc,cngInjectionTime,cngIstValue,cngRailPressure,cngTurboPressure,cngLevel,cngRailPressure,cngWaterTemperature,cngRailTemperature,cngTripFuel,cngTotalFuelUsed,cngDieselReduction,error");
  GP.TITLE("Дисплей EVO", "");
  GP.NAV_TABS("Телеметрия,Настройка");
  // GP.FORM_BEGIN("");
  // GP.FORM_BEGIN();
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

void actionPage1()
{
  if (ui.update())
  {
    ui.updateInt("canSpeed", data_can.canSpeed);
    ui.updateString("vehicleType", data_can.vehicleType);
    ui.updateInt("tankVolume", data_can.tankVolume);

    if (counter_lost_can_EVO < 50)
    {
      String str = data_can.state ? "Газ" : "Дт";
      auto t = calculator(data_can);
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

    if (counter_lost_can_vehicle++ < 50)
    {
      int d = data_can.distance / 1000;
      ui.updateInt("distance", d);
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

void twai_generate_filter(uint16_t min, uint16_t max, twai_filter_config_t *filter)
{

  uint32_t mask[2] = {0, 0};

  for (uint16_t i = min; i < max; i++)
  {
    mask[0] &= i;
    mask[1] &= ~i;
  }

  filter->acceptance_mask = ~((mask[0] | mask[1]) << 21);
  filter->acceptance_code = (min & (mask[0] | mask[1])) << 21;
  filter->single_filter = true;
}
