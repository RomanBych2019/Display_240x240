#pragma once
#include <Arduino.h>
#include "set"

struct distanceLPG
{
    uint32_t begin;  //  начало отсчета пробега
    uint32_t result; //   итоговый пробег
};

struct Can_Data
{
    uint8_t state = 0;              //  Режим дизель - 0, смешанный - 1
    uint8_t cngInjectionTime;       //  Время впрыска газа, 0.1 мсек
    uint8_t cngIstValue = 0;        //  Мгновенный расход газа, 0.1 кг/час
    uint8_t cngDieselReduction;     //  карта дизель
    uint8_t cngTurboPressure;       // давление турбо
    uint8_t cngRailPressure;        //  Давлениев газа в рампе форсунок, 0.02 бар
    int8_t cngRailTemperature;      //  Температура газа в рампе форсунок, 0.1 С -40С
    uint32_t hoursindiesel;         //  Время работы на ДТ, 1 мин
    uint32_t hoursindualfuel;       //  Время работы в смешанном режиме, 1 мин
    uint8_t cngLevel;               //  Давление в балонах,  бар
    uint8_t cngWaterTemperature;    //  Температура редуктора, 0.1 С -40С
    uint32_t cngTripFuel;           //  Расход газа за поездку, 1 гр
    uint32_t cngTotalFuelUsed;      //  Общий расход газа, 0.5 кг
    uint8_t ACC;                    //   Педаль акселератора, %
    uint16_t tankVolume = 150;      //   Объем баллонов, л
    uint8_t errorEVO[6];            //   Ошибки EVO
    uint8_t verID;                  //   Version ID High
    uint8_t verFMlow;               //   Version FW Low

    uint32_t rpm = 0;               //  Обороты двигателя
    uint32_t wheelSpeed = 0;        //  Скорость автомобиля
    uint32_t oilFuelRate = 0;       //  Мгновенный расход ДТ литр/час
    uint32_t engineLoad = 0;        //  нагрузка двигатель
    uint32_t cruise = 0;            //  круиз - вкл - 1, выкл - 0
    uint32_t distance = 0;          //  пробег автомобиля
    int32_t airTemper;              //  температура воздуха
    int32_t oilTemper;              //  температура масла
    int32_t engineTemper;           //  температура двигателя
    int32_t fuelTemper;             //  температура топлива
    int32_t exhaustGasTemper;       //  температура выхлопных газов
    int32_t vehicleWeight;          //  вес автомобиля

    String vehicleType{};           // тип автомобиля
    uint16_t canSpeed{};            // корость can-шины

    uint8_t levelEconomicalDriving; //  уровень экономичного вождения (0-100%)
    uint32_t travelDistance;
    distanceLPG distLPG;
};


const uint32_t PGN1 = 0x18FFF0BB;         // газ
const uint32_t PGN2 = 0x18FFF1BB;         // газ
const uint32_t PGN3 = 0x18FFFBBB;         // газ - ошибки
const uint32_t PGN4 = 0x18FFF8BB;         // газ
const uint32_t PGN5 = 0x18FEF100;         // скорость, круиз-контроль, пробег авто в J1939
const uint32_t PGN6 = 0x18F00400;         // обороты авто в J1939
const uint32_t PGN7 = 0x18FEF200;         // мгновенный расход ДТ авто в J1939
const uint32_t PGN8 = 0x0CF00400;         // нагрузка на двигатель в J1939
const uint32_t PGN9 = 0x18FEC1EE;         // пробег авто в SHAKMAN
const uint32_t PGN10 = 0x18FEF600;        // температура воздуха, выхлопных газов в J1939
const uint32_t PGN11 = 0x18FEEE00;        // температура ОЖ, масла, топлива в J1939
const uint32_t PGN12 = 0x18FEEA00;        // вес в J1939

const std::set<unsigned long> FILTR_ID{0x18FFF0BB, 0x18FFF1BB, 0x18FFFBBB, 0x18FFF8BB,
                                       0x18FEF100, 0x18F00400, 0x18FEF200, 0x0CF00400,
                                       0x18FEC1EE, 0x18FEF600, 0x18FEEE00, 0x18FEEA00};