#include "Valve.h"
#include <limits.h> //TODO what is this for
#include "Utility.h"

Valve::Valve(): Valve(INVALID_PIN, 0, 0, 0, 0)
{
    
} 
Valve::Valve(const Valve::Data& data) : Valve(data.valveNumber, data.hour, data.minute, data.daysOn, data.timeCountdown)
{
    
}



//TODO figure out what happens when timeCountdown is a large value(ie. prevent the overflow that will happen on checking the valve to open or close)
Valve::Valve(int8_t valveNumber, uint8_t hour, uint8_t minute, uint8_t daysOn, uint16_t timeCountdown):
    m_data{valveNumber, hour, minute, daysOn, timeCountdown}, m_turnedOnTime{-1}
{
    validate();
}


bool Valve::isValvePinValid()
{
    return m_data.valveNumber >= LOWEST_VALID_PIN_FOR_VALVE && m_data.valveNumber <= HIGHEST_VALID_PIN_FOR_VALVE;

}
void Valve::validate()
{
    
    if(!isValvePinValid()
    //don't want to turn on valves if they don't have any days to be turned on
    || m_data.daysOn == 0
    /*
    this is a special case, and is needed to prevent the program from constantly trying to turn on, and off the valve
    if the timeCountdown is set to 0;
    and, of course, we don't want a negative timeCountdown...
    */
    || m_data.timeCountdown < 1)
    {
        m_data.valveNumber = INVALID_PIN;
        return;
    }
    pinMode(m_data.valveNumber, OUTPUT);
    digitalWrite(m_data.valveNumber, HIGH);
}

bool Valve::isDayOn(int day) const
{
        if(day < 1 || day > 7)
        {
            return false;
        }
        return (m_data.daysOn >> day) & 0x1;
}

/**
 * @dt current date time to check against
 * @return true if we need to turn off this valve, false otherwise
 **/
bool Valve::checkTurnOff(const DateTime& dt) const
{
    if(!isOn())
    {
        return false;//no need to turn it off, if it is already off
    }
    
    return checkTurnOffTime(dt) <= 0;
}

/**
 * @dt current date time to check against
 * @return true if we need to turn on this valve, false otherwise
 **/
bool Valve::checkTurnOn(const DateTime& dt) const
{
    if(isOn())
    {
        return false;//no need to turn it on, if it is already on
    }
    return checkTurnOnTime(dt) <= 0;
}


/**
 * @dt current date time to check against
 * @return minutes left before the valve needs to be turned off. Negative values indicate the valve should have been  switched off before n minutes
 **/
int Valve::checkTurnOffTime(const DateTime& dt) const
{
    int currentMinute = Utility::dateTimeToMinutesInWeek(dt); 
    
    int num = currentMinute - m_data.timeCountdown;
    if(num <= 0)
    {
        num = MINUTES_IN_WEEK + num;
    }
    
    return m_turnedOnTime - num;
}

/**
 * @dt current date time to check against
 * @return minutes left before the valve needs to be turned on. Negative values indicate the valve should have been switched on before n minutes
 **/
int Valve::checkTurnOnTime(const DateTime& dt) const
{
    int currentMinute = Utility::dateTimeToMinutesInWeek(dt); 
    int soonestTurnOn = INT_MAX;
    for(int i = 0; i < 7; ++i)
    {
        if(isDayOn(i+1))
        {
            int turnOnTime = i * MINUTES_IN_DAY + m_data.hour * 60 + m_data.minute;
            int turnOnDelay = turnOnTime - currentMinute;
            if(turnOnDelay < 0 && (abs(turnOnDelay) >= m_data.timeCountdown))
            {
                turnOnDelay = MINUTES_IN_WEEK + turnOnDelay;
            }
            if(turnOnDelay < soonestTurnOn)
            {
                soonestTurnOn = turnOnDelay;
            }
			//this part gets executed
        }
    }
    return soonestTurnOn;
}

int Valve::getActionTime(const DateTime& dt) const
{
    if(isOn())
    {
        return checkTurnOffTime(dt);
    }
    else
    {
        return checkTurnOnTime(dt);
    }
}

void Valve::turnOn(const DateTime& dt)
{
    m_turnedOnTime = Utility::dateTimeToMinutesInWeek(dt);
    switchValve();
}

void Valve::turnOff()
{
    m_turnedOnTime = -1;
    switchValve();
}

int Valve::getValveNumber() const
{
    return m_data.valveNumber;
}

void Valve::switchValve()
{
    if(m_data.valveNumber == INVALID_PIN)
    {
        return;
    }
    digitalWrite(m_data.valveNumber, LOW);
    Utility::delay(LATCH_TIME_MILLIS);
    digitalWrite(m_data.valveNumber, HIGH);
}

Message* Valve::toMessage() const
{
	Message *msg = new Message(Message::Type::request, Message::Action::valve, Message::Info::none, (uint8_t)Valve::NETWORK_SIZE);

	uint16_t timeCountdown = htons(m_data.timeCountdown);

	(*msg)[0]																			= m_data.valveNumber;
	(*msg)[sizeof(m_data.valveNumber)]													= m_data.hour;
	(*msg)[sizeof(m_data.valveNumber) + sizeof(m_data.hour)]							= m_data.minute;
	(*msg)[sizeof(m_data.valveNumber) + sizeof(m_data.hour) + sizeof(m_data.minute)]	= m_data.daysOn;
	memcpy(&(msg->m_data[sizeof(m_data.valveNumber) + sizeof(m_data.hour) + sizeof(m_data.minute) + sizeof(m_data.daysOn)]), &timeCountdown, sizeof(timeCountdown));

	return msg;
}

void Valve::fromMessage(const Message& msg)
{
	m_data.valveNumber =	msg[0];
	m_data.hour =			msg[sizeof(m_data.valveNumber)];
	m_data.minute =			msg[sizeof(m_data.valveNumber) + sizeof(m_data.hour)];
	m_data.daysOn =			msg[sizeof(m_data.valveNumber) + sizeof(m_data.hour) + sizeof(m_data.minute)];
	m_data.timeCountdown = *((uint16_t*)(&(msg[sizeof(m_data.valveNumber) + sizeof(m_data.hour) + sizeof(m_data.minute) + sizeof(m_data.daysOn)])));

	m_data.timeCountdown = ntohs(m_data.timeCountdown);
}

bool Valve::isOn() const
{
	return m_turnedOnTime >= 0;
}
