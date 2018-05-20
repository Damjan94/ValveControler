#include "Valve.h"

int8_t Valve::hBridgePin[2] = {INVALID_PIN, INVALID_PIN};
int8_t Valve::hBridgeState = -1;
bool Valve::isHbridgeSet = false;

Valve::Valve(): Valve(INVALID_PIN, 0, 0, 0, 0)
{
    
} 
Valve::Valve(Valve::data data) : Valve(data.valveNumber, data.hour, data.minute, data.timeCountdown, data.daysOn)
{
    
}
Valve::Valve(int8_t valveNumber, uint8_t hour, uint8_t minute, uint16_t timeCountdown, uint8_t daysOn):
    m_data{timeCountdown, valveNumber, hour, minute, daysOn}, m_turnedOnTime{-1}
{
    if(!isValvePinValid(m_data.valveNumber))
    {
        //pins less than LOWEST_VALID_PIN_FOR_VALVE are used by arduino for bluetooth
        //pins grater than HIGHEST_VALID_PIN_FOR_VALVE are used for interrupts, and hbridge
        m_data.valveNumber = INVALID_PIN;
        return;
    }
    pinMode(valveNumber, OUTPUT);
    digitalWrite(valveNumber, HIGH);
}

bool Valve::isDayOn(int day) const
{
        if(day < 1 || day > 7)
        {
            return false;
        }
        return (m_data.daysOn >> day) & 0x1;
}

void Valve::setDayOn(int day, bool value)
{
        if(day < 1 || day > 7)
        {
            return;
        }
        uint8_t newbit = !!value;    // Also booleanize to force 0 or 1
        m_data.daysOn ^= (-newbit ^ m_data.daysOn) & (0x1 << day);
} 

bool Valve::isValvePinValid(int8_t pinNumber)
{
    return pinNumber >= LOWEST_VALID_PIN_FOR_VALVE && pinNumber <= HIGHEST_VALID_PIN_FOR_VALVE;
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
    int currentMinute = (dt.Dow-1) * MINUTES_IN_DAY + dt.Hour * 60 + dt.Minute;
    int turnOffMinute = m_turnedOnTime + m_data.timeCountdown;
    //if current minute = 10 000 and turn off minute is 10 081 % 10 080 = 1, it will turn it off without waiting for the current minute to overflow to 0
    if(turnOffMinute >= MINUTES_IN_WEEK)
    {
        int turnedOnDow = (m_turnedOnTime / MINUTES_IN_DAY) + 1;
        //in arduino 1439/1440 = 0
        //1441 / 1440 = 1
        if( turnedOnDow != dt.Dow && (currentMinute + MINUTES_IN_WEEK) >= turnOffMinute)
        {
            return true;
        }
    }
    // on the other hand, if no '%' is used, than it wil never turn it off, since current minute will never be 10 081...
    if(currentMinute >= turnOffMinute)
    {
        return true;
    }
    
    return false;
}

void Valve::turnOff()
{
    m_turnedOnTime = -1;
    switchValve(LOW);
}

int Valve::getValveNumber() const
{
    return m_data.valveNumber;
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
    if(!isDayOn(dt.Dow))//if it is sunday 00:05, and it is set to turn on on saturday 23:59, it won't turn on
    {
        return false;
    }
    int currentMinute = (dt.Dow - 1) * MINUTES_IN_DAY + dt.Hour * 60 + dt.Minute;
    int turnOnMinuteDay = m_data.hour * 60 + m_data.minute;
    
    int turnOnTime = (dt.Dow - 1) * MINUTES_IN_DAY + turnOnMinuteDay;
    int turnOffTime = turnOnTime + m_data.timeCountdown;
    if(turnOnTime - currentMinute <= 0 && turnOffTime - currentMinute > 0)
    {
        return true;
    }
    
    return false;
}

void Valve::turnOn(const DateTime& dt)
{
    m_turnedOnTime = ((dt.Dow-1) * MINUTES_IN_DAY) + m_data.hour*60 + m_data.minute;/*(dt.Hour*60) + dt.Minute;*/
    switchValve(HIGH);
}

void Valve::switchValve(int newHbridgeState)
{
    if(m_data.valveNumber == INVALID_PIN)
    {
        return;
    }
    
    bool switchSuccessful = switchHBridge(newHbridgeState);
    
    if(!switchSuccessful)
    {
        return;
    }
    delay(RELAY_SETTLE_TIME);
    
    digitalWrite(m_data.valveNumber, LOW);
    delay(LATCH_TIME_MILLIS);
    digitalWrite(m_data.valveNumber, HIGH);
}

bool Valve::switchHBridge(int8_t state)
{
    if(!isHbridgeSet)
    {
        return false;
    }
    digitalWrite(hBridgePin[0], state);
    digitalWrite(hBridgePin[1], state);
    hBridgeState = state;
    return true;
}

int8_t Valve::getHBridgeState()
{
    return hBridgeState;
}

void Valve::setHBridgePin(const int8_t pinNum[])
{
    if(sizeof(pinNum) / sizeof(int8_t) != 2)
    {
        return;
    }
    if(pinNum[0] < 0 || pinNum[1] < 0)
    {
        return;
    }
    if(isValvePinValid(pinNum[0]) || isValvePinValid(pinNum[1]))
    {
        return;//hbridge pins can not be in range of valve pins
    }
    
    hBridgePin[0] = pinNum[0];
    hBridgePin[1] = pinNum[1];
    
    pinMode(hBridgePin[0], OUTPUT);
    pinMode(hBridgePin[1], OUTPUT);
    isHbridgeSet = true;
}

void Valve::fromSerial(HardwareSerial& serial)
{
    uint8_t valveBytes[VALVE_NETWORK_SIZE];
    int readByteCount = serial.readBytes(valveBytes, VALVE_NETWORK_SIZE);
    int8_t valveNumber = valveBytes[0];
    uint8_t hour = valveBytes[1];
    uint8_t minute = valveBytes[2];
    uint8_t daysOn = valveBytes[3];
    uint16_t timeCountdown =  *((uint16_t*) (void*) &valveBytes[4]);

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

bool Valve::isOn() const
{
    return m_turnedOnTime >= 0;
}

int8_t* Valve::getHBridgePin()
{
    return hBridgePin;
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
