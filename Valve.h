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
        int8_t valveNumber;
        uint8_t hour;
        uint8_t minute;
        uint8_t daysOn;//days are indexed from 1 to 7(sun-sat)
        uint16_t timeCountdown;
    };
private:
    Valve(int8_t valveNumber, uint8_t hour, uint8_t minute, uint8_t daysOn, uint16_t timeCountdown);
    Valve::data m_data;
    int16_t m_turnedOnTime;//negative means that the valve is off, positive value indicates what minute in the week did it turn on

    const static uint64_t LATCH_TIME_MILLIS = 15;//default 15
    const static uint16_t MINUTES_IN_DAY = 1440;
    const static uint16_t MINUTES_IN_WEEK = 10080;
    const static int8_t LOWEST_VALID_PIN_FOR_VALVE = 5;
    const static int8_t HIGHEST_VALID_PIN_FOR_VALVE = 12;
    const static int8_t INVALID_PIN = -1;

    void switchValve();
    bool isValvePinValid();
    void validate();

    int checkTurnOnTime(const DateTime& dt) const;
    int checkTurnOffTime(const DateTime& dt) const;

public:
    const static int NETWORK_SIZE = sizeof(data::valveNumber) + sizeof(data::hour) + sizeof(data::minute) + sizeof(data::daysOn) + sizeof(data::timeCountdown);//sizeof(Valve::data); << cant do that coz of padding
    Valve(); 
    Valve(const data&);
    Valve(const uint8_t* data, bool isDataInNetworkByteOrder = false);
    bool isDayOn(int day) const;
    //void setDayOn(int day, bool value);
	/**
	these two functions assume that the hbridge is set properly
	**/
    void turnOn(const DateTime& dt);
    void turnOff();

    bool checkTurnOn(const DateTime& dt) const;
    bool checkTurnOff(const DateTime& dt) const;
    //void fromSerial(HardwareSerial& serial);
    //void toSerial(HardwareSerial& serial) const;
    bool isOn() const;
    int getValveNumber() const;
    uint8_t* toBytes(bool isDataInNetworkByteOrder = false) const;
    data fromBytes(const uint8_t* bytes, bool isDataInNetworkByteOrder = false);
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
