#pragma once
#include <Arduino.h>
#include <DS3231_Simple.h>
#include "message.h"

class Valve
{
    
public:
    struct Data
    {
        int8_t valveNumber;
        uint8_t hour;
        uint8_t minute;
        uint8_t daysOn;//days are indexed from 1 to 7(sun-sat)
        uint16_t timeCountdown;
    };
	
private:
	Valve(int8_t valveNumber, uint8_t hour, uint8_t minute, uint8_t daysOn, uint16_t timeCountdown);
    Valve::Data m_data;
    int16_t m_turnedOnTime;//negative means that the valve is off, positive value indicates what minute in the week did it turn on

    const static uint64_t LATCH_TIME_MILLIS = 15;//default 15
    const static uint16_t MINUTES_IN_DAY = 1440;
    const static uint16_t MINUTES_IN_WEEK = 10080;
    const static int8_t LOWEST_VALID_PIN_FOR_VALVE = 5;
    const static int8_t HIGHEST_VALID_PIN_FOR_VALVE = 12;
    const static int8_t INVALID_PIN = -1;

    void switchValve();

    void validate();

    int checkTurnOnTime(const DateTime& dt) const;
    int checkTurnOffTime(const DateTime& dt) const;

public:
    const static size_t NETWORK_SIZE = sizeof(Data::valveNumber) + sizeof(Data::hour) + sizeof(Data::minute) + sizeof(Data::daysOn) + sizeof(Data::timeCountdown);//sizeof(Valve::data); << cant do that coz of padding
    Valve(); 
    Valve(const Data&);
    bool isDayOn(int day) const;

	/**
	these two functions assume that the hbridge is set properly
	**/
    void turnOn(const DateTime& dt);
    void turnOff();

    bool checkTurnOn(const DateTime& dt) const;
    bool checkTurnOff(const DateTime& dt) const;
    bool isOn() const;
	bool isValvePinValid() const;
    int getValveNumber() const;

	Message* toMessage() const;
	void fromMessage(const Message & msg);

    /**
     * @param dt the time from which to calculate the soonest action time
     * @return the number of minutes in which the action needs to be taken. Negative indecates that the action should have been taken before that amount of minutes
     **/
    int getActionTime(const DateTime& dt) const;
};

