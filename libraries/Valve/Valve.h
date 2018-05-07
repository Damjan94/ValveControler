#ifndef VALVE_H
#define VALVE_H

//#define DEBUG

#include <Arduino.h>
#include <DS3231_Simple.h>
class Valve
{
private:
    int8_t m_valveNumber;
    uint8_t m_hour;
    uint8_t m_minute;
    uint16_t m_timeCountdown;
    int16_t m_turnedOnTime;//negative means that the valve is off, positive value indicates what minute in the week did it turn on
    uint8_t m_daysOn;//days are indexed from 1 to 7(sun-sat)
    
    static int hBridgePin;
    const static long RELAY_SETTLE_TIME = 5;
    const static long LATCH_TIME_MILLIS = 15;
    const static int MINUTES_IN_DAY = 1440;
    const static int MINUTES_IN_WEEK = 10080;
    const static int LOWEST_VALID_PIN_FOR_VALVE = 5;
    const static int HIGHEST_VALID_PIN_FOR_VALVE = 13;
    const static int INVALID_VALVE = -1;

    void switchValve(int newHbridgeState);
public:
    const static int VALVE_NETWORK_SIZE = 6;
    Valve(); 
    Valve(int8_t valveNumber, uint8_t hour, uint8_t minute, uint16_t timeCountdown, uint8_t daysOn);
    bool isDayOn(int day) const;
    void setDayOn(int day, bool value);
    void turnOn(const DateTime& dt);
    void turnOff();
    bool checkTurnOn(const DateTime& dt) const;
    bool checkTurnOff(const DateTime& dt) const;
    void fromSerial(HardwareSerial& serial);
    void toSerial(HardwareSerial& serial) const;
    bool isOn() const;
    int getValveNumber() const;
    static void setHBridgePin(int pinNum);
#ifdef DEBUG
    String toString();
#endif //DEBUG
};
#endif
