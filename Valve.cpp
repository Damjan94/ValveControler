#include "Valve.h"
#include <limits.h> //TODO what is this for
#include <util.h>

Valve::Valve(): Valve(INVALID_PIN, 0, 0, 0, 0)
{
    
} 
Valve::Valve(const Valve::data& data) : Valve(data.valveNumber, data.hour, data.minute, data.daysOn, data.timeCountdown)
{
    
}
Valve::Valve(const uint8_t* data, bool isDataInNetworkByteOrder) : Valve(fromBytes(data, isDataInNetworkByteOrder))
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
/*
void Valve::setDayOn(int day, bool value)
{
        if(day < 1 || day > 7)
        {
            return;
        }
        uint8_t newbit = !!value;    // Also booleanize to force 0 or 1
        m_data.daysOn ^= (-newbit ^ m_data.daysOn) & (0x1 << day);
} 
*/



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
    int currentMinute = utility::dateTimeToMinutesInWeek(dt); 
    
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
    int currentMinute = utility::dateTimeToMinutesInWeek(dt); 
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
    m_turnedOnTime = utility::dateTimeToMinutesInWeek(dt);
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
    utility::delay(LATCH_TIME_MILLIS);
    digitalWrite(m_data.valveNumber, HIGH);
}
/*
void Valve::fromSerial(HardwareSerial& serial)
{
    uint8_t valveBytes[VALVE_NETWORK_SIZE];
    int readByteCount = serial.readBytes(valveBytes, VALVE_NETWORK_SIZE);
    if(readByteCount != VALVE_NETWORK_SIZE){};//TODO Handle the bad read
    int8_t valveNumber = valveBytes[0];
    uint8_t hour = valveBytes[1];
    uint8_t minute = valveBytes[2];
    uint8_t daysOn = valveBytes[3];
    //uint16_t timeCountdown =  *((uint16_t*) (void*) &valveBytes[4]);
    uint16_t timeCountdown;
    memcpy(&timeCountdown, &valveBytes[4], sizeof(timeCountdown));

    (*this) = Valve(valveNumber, hour, minute, timeCountdown, daysOn);
}

void Valve::toSerial(HardwareSerial& serial) const
{
    serial.write(m_data.valveNumber);
    serial.write(m_data.hour);
    serial.write(m_data.minute);
    serial.write(m_data.daysOn);
    serial.write((uint8_t)(m_data.timeCountdown>>1*8));
    serial.write((uint8_t)(m_data.timeCountdown>>0*8));
}
*/

Valve::data Valve::fromBytes(const uint8_t* bytes, bool isDataInNetworkByteOrder)
{
    data dt;
    
    dt.valveNumber				= bytes[0];
    dt.hour						= bytes[sizeof(dt.valveNumber)];
    dt.minute					= bytes[sizeof(dt.valveNumber) + sizeof(dt.hour)];
    dt.daysOn					= bytes[sizeof(dt.valveNumber) + sizeof(dt.hour) + sizeof(dt.minute)];
    uint16_t timeCountdown		= *((uint16_t*) (&(bytes[sizeof(dt.valveNumber) + sizeof(dt.hour) + sizeof(dt.minute) + sizeof(dt.daysOn)])));
    
    if(isDataInNetworkByteOrder == true)
    {
        dt.timeCountdown = ntohs(timeCountdown);
    }
    return dt;
}

uint8_t* Valve::toBytes(bool isDataInNetworkByteOrder) const
{
    uint16_t timeCountdown = m_data.timeCountdown;
    if(isDataInNetworkByteOrder == true)
    {
        timeCountdown = htons(timeCountdown);
    }
    
    uint8_t* bytes = (uint8_t*)malloc(Valve::NETWORK_SIZE);
    bytes[0] =                                                                                                      m_data.valveNumber;
    bytes[sizeof(m_data.valveNumber)] =                                                                             m_data.hour;
    bytes[sizeof(m_data.valveNumber) + sizeof(m_data.hour)] =                                                       m_data.minute;
    bytes[sizeof(m_data.valveNumber) + sizeof(m_data.hour) + sizeof(m_data.minute)] =                               m_data.daysOn;
    //bytes[sizeof(m_data.valveNumber) + sizeof(m_data.hour) + sizeof(m_data.minute) + sizeof(m_data.daysOn)] =       (uint8_t) (timeCountdown >> 8);
    //bytes[sizeof(m_data.valveNumber) + sizeof(m_data.hour) + sizeof(m_data.minute) + sizeof(m_data.daysOn) + 1] =	(uint8_t) (timeCountdown);
	memcpy(&(bytes[sizeof(m_data.valveNumber) + sizeof(m_data.hour) + sizeof(m_data.minute) + sizeof(m_data.daysOn)]), &timeCountdown, sizeof(timeCountdown));
    return bytes;
}
bool Valve::isOn() const
{
    return m_turnedOnTime >= 0;
}


/////////////////////////////////////////////////////////////////////
//utility functions, move it to it's own file
void utility::delay(unsigned long ms)
{
    unsigned long startTime = millis();
    while((millis() - startTime) < ms);
}

int utility::dateTimeToMinutesInWeek(const DateTime& dt)
{
    static const int MINUTES_IN_DAY = 1440;
    return ((dt.Dow-1) * MINUTES_IN_DAY) + (dt.Hour * 60) + dt.Minute;
}
#include <time.h>
DateTime utility::addMinutesToDate(int minutes, const DateTime& date)
{
    struct tm timeStructure;
    timeStructure.tm_sec = date.Second;
    timeStructure.tm_min = date.Minute;
    timeStructure.tm_hour = date.Hour;
    timeStructure.tm_mday = date.Day;
    timeStructure.tm_mon = date.Month-1;
    timeStructure.tm_year = date.Year+100;
    timeStructure.tm_wday = date.Dow-1;
    timeStructure.tm_yday = 0;
    timeStructure.tm_isdst = -1;

    time_t calcTime = mktime(&timeStructure);
    calcTime += minutes;
    struct tm tmStruct = *localtime(&calcTime);//<<this is statically allocated. DO NOT free it!

    DateTime newTime;
    newTime.Second = tmStruct.tm_sec;
    newTime.Minute = tmStruct.tm_min;
    newTime.Hour = tmStruct.tm_hour;
    newTime.Day = tmStruct.tm_mday;
    newTime.Month = tmStruct.tm_mon+1;
    newTime.Year = tmStruct.tm_year-100;
    newTime.Dow = tmStruct.tm_wday+1;

    return newTime;
}

#ifdef DEBUG
String Valve::toString()
{
    static String nl = F("\n");
    String str = "";
    str += F("Valve Number: ");
    str += valveNumber;
    str += nl;
    str += F("Turning on: ");
    for(int i = 0; i < 7;++i)
    {
        if(isDayOn(i))
        {
            switch(i)
            {
                case 0:
                {
                    str += F("Mon, ");
                    break;
                }
                case 1:
                {
                    str += F("Tue, ");
                    break;
                }
                case 2:
                {
                    str += F("Wed, ");
                    break;
                }
                case 3:
                {
                    str += F("Thu, ");
                    break;
                }
                case 4:
                {
                    str += F("Fri, ");
                    break;
                }
                case 5:
                {
                    str += F("Sat, ");
                    break;
                }
                case 6:
                {
                    str += F("Sun,");
                    break;
                }
            }
        }
    }
    str += nl;
    str += F("Hour: ");
    str += m_data.hour;
    str += nl;
    str += F("Minute: ");
    str += m_data.minute;
    str += nl;
    str += F("Time Countdown: ");
    str += m_data.timeCountdown;
    str += nl;
    str += F("Is On: ");
    str += ((m_turnedOnTime < 0) ? F("false"):F("true"));
    return str;
}

#endif //DEBUG
