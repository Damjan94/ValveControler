#ifndef VALVE_H
#define VALVE_H

//#define DEBUG

#include <Arduino.h>
#include <DS3231_Simple.h>
class Valve
{
    
public:
    struct data
    {
        uint16_t timeCountdown;
        int8_t valveNumber;
        uint8_t hour;
        uint8_t minute;
        uint8_t daysOn;//days are indexed from 1 to 7(sun-sat)
    };
private:
    
    Valve::data m_data;
    int16_t m_turnedOnTime;//negative means that the valve is off, positive value indicates what minute in the week did it turn on
    
    const static long LATCH_TIME_MILLIS = 15;
    const static int MINUTES_IN_DAY = 1440;
    const static int MINUTES_IN_WEEK = 10080;
    const static int LOWEST_VALID_PIN_FOR_VALVE = 5;
    const static int HIGHEST_VALID_PIN_FOR_VALVE = 12;
    const static int INVALID_PIN = -1;

    void switchValve();
    bool isValvePinValid();
    
    int checkTurnOnTime(const DateTime& dt) const;
    int checkTurnOffTime(const DateTime& dt) const;
public:
    const static int VALVE_NETWORK_SIZE = 6;//sizeof(Valve::data); << cant do that coz of padding
    Valve(); 
    Valve(Valve::data data);
    Valve(int8_t valveNumber, uint8_t hour, uint8_t minute, uint16_t timeCountdown, uint8_t daysOn);
    bool isDayOn(int day) const;
    //void setDayOn(int day, bool value);
    void turnOn(const DateTime& dt);
    void turnOff();
    bool checkTurnOn(const DateTime& dt) const;
    bool checkTurnOff(const DateTime& dt) const;
    void fromSerial(HardwareSerial& serial);
    void toSerial(HardwareSerial& serial) const;
    bool isOn() const;
    int getValveNumber() const;
    /**
     * @param dt the time from which to calculate the soonest action time
     * @return the number of minutes in which the action needs to be taken. Negative indecates that the action should have been taken before that amount of minutes
     **/
    int getActionTime(const DateTime& dt) const;
#ifdef DEBUG
    String toString();
#endif //DEBUG
};
//TODO move this to a seperate file
namespace utility
{
    void delay(unsigned long ms);
    int dateTimeToMinutesInWeek(const DateTime& dt);
    DateTime addMinutesToDate(int minutes, const DateTime& date);
};
#endif
