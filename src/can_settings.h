#pragma once
#include <Arduino.h>
#include "set"

struct distanceLPG
{
    uint32_t begin;    //  начало отсчета пробега на газовом топливе в поездке
    uint32_t result;   //  пробег на газовом топливе в поездке
    float gas_mileage; //  оставшийся пробег на газе
};

struct Can_Data
{
    uint8_t state = 0;           //  Режим дизель - 0, смешанный - 1
    uint8_t cngInjectionTime;    //  Время впрыска газа, 0.1 мсек
    uint8_t cngIstValue = 0;     //  Мгновенный расход газа, 0.1 кг/час
    uint8_t cngDieselReduction;  //  карта дизель
    uint8_t cngTurboPressure;    // давление турбо
    uint8_t cngRailPressure;     //  Давлениев газа в рампе форсунок, 0.02 бар
    int8_t cngRailTemperature;   //  Температура газа в рампе форсунок, 0.1 С -40С
    uint32_t hoursindiesel;      //  Время работы на ДТ, 1 мин
    uint32_t hoursindualfuel;    //  Время работы в смешанном режиме, 1 мин
    uint8_t cngLevel;            //  Давление в балонах,  бар
    uint8_t cngWaterTemperature; //  Температура редуктора, 0.1 С -40С
    uint32_t cngTripFuel;        //  Расход газа за поездку, 1 гр
    uint32_t cngTotalFuelUsed;   //  Общий расход газа, 0.5 кг
    uint8_t ACC;                 //   Педаль акселератора, %
    uint16_t tankVolume = 150;   //   Объем баллонов, м3 (из файла настройки)
    uint8_t errorEVO[6];         //   Ошибки EVO
    uint8_t verID;               //   Version ID High
    uint8_t verFMlow;            //   Version FW Low

    uint32_t rpm = 0;         //  Обороты двигателя
    uint32_t wheelSpeed = 0;  //  Скорость автомобиля
    uint32_t oilFuelRate = 0; //  Мгновенный расход ДТ литр/час
    uint32_t engineLoad = 0;  //  нагрузка двигатель
    uint32_t cruise = 0;      //  круиз - вкл - 1, выкл - 0
    uint32_t distance = 0;    //  пробег автомобиля
    int32_t airTemper;        //  температура воздуха
    int32_t oilTemper;        //  температура масла
    int32_t engineTemper;     //  температура двигателя
    int32_t fuelTemper;       //  температура топлива
    int32_t exhaustGasTemper; //  температура выхлопных газов
    int32_t vehicleWeight;    //  вес автомобиля

    int16_t lls = 0;         //  уровень ДУТа в литрах
    int16_t full_tank = 100; //  объем бака в литрах

    String vehicleType{}; // тип автомобиля
    uint16_t canSpeed{};  // скорость can-шины

    uint8_t levelEconomicalDriving; //  уровень экономичного вождения (0-100%)
    // uint32_t travelDistance;
    distanceLPG distLPG;
    float average_gasconsumption; //  средний расход газа на 100 км
};

const uint32_t PGN1 = 0x18FFF0BB;  // газ
const uint32_t PGN2 = 0x18FFF1BB;  // газ
const uint32_t PGN3 = 0x18FFFBBB;  // газ - ошибки
const uint32_t PGN4 = 0x18FFF8BB;  // газ
const uint32_t PGN5 = 0x18FEF100;  // скорость, круиз-контроль,
const uint32_t PGN6 = 0x18FEC100;  // пробег авто в J1939
const uint32_t PGN7 = 0x18FEF200;  // мгновенный расход ДТ авто в J1939
const uint32_t PGN8 = 0x0CF00400;  // нагрузка на двигатель в J1939
const uint32_t PGN9 = 0x18FEC1EE;  // пробег авто в SHAKMAN
const uint32_t PGN10 = 0x18FEF600; // температура воздуха, выхлопных газов в J1939
const uint32_t PGN11 = 0x18FEEE00; // температура ОЖ, масла, топлива в J1939
const uint32_t PGN12 = 0x18FEEA2F; // вес в J1939
const uint32_t PGN13 = 0x18FEEA00; //  данные от терминала Смарт

const uint32_t PGN_SEND_ON_EVO = 0x18FFEEDD; // отправка нажатия кнопки включения/выключения газа
const uint32_t PGN_SEND_DATA = 0x18FFFEDD;   // отправка диагностической информации для мониторинга

const std::set<uint32_t> FILTR_ID{PGN1, PGN2, PGN3, PGN4,
                                  PGN5, PGN6, PGN7, PGN8,
                                  PGN9, PGN10, PGN11, PGN13};