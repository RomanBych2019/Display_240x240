#pragma once
#include <Arduino.h>

enum class SolenoidHealth : uint8_t
{
    OK,
    OPEN_CIRCUIT,         // включили, тока нет/мало
    OVERLOAD,             // ток слишком большой
    MOSFET_LEAK_OR_SHORT, // ток есть при OFF
    SHORT,                // зафиксировано короткое замыкание
};

class Valve
{
private:
    struct Val
    {
        SolenoidHealth heat{};
        int current = 0;    //  ток потребления клапана, А
        bool state = false; //  состояние соленоида - открыт (газ идет) или закрыт (газа нет)
    };

    Val val_;

public:
    Valve() = default;
    
    Valve(SolenoidHealth h, int c, bool st = false)
    {
        val_.heat = h;
        val_.current = c;
        val_.state = st;
    }  
    
    void updateState(SolenoidHealth health, int current, bool state = false)
    {
        val_.heat = health;
        val_.current = current;
        val_.state = state;
    }
    
    Val getState()
    {
        return val_;
    }
};