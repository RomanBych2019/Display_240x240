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
        int current = 0;
    };

    Val val_;

public:
    Valve() = default;
    
    Valve(SolenoidHealth h, int c)
    {
        val_.heat = h;
        val_.current = c;
    }  
    
    void updateState(SolenoidHealth health, int current)
    {
        val_.heat = health;
        val_.current = current;
    }
    
    Val getState()
    {
        return val_;
    }
};