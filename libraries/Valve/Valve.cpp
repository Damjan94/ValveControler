#include "Valve.h"
int Valve::hBridgePin = -1;

Valve::Valve(): Valve(INVALID_VALVE, 0, 0, 0, 0)
{
    
} 

Valve::Valve(int8_t valveNumber, uint8_t hour, uint8_t minute, uint16_t timeCountdown, uint8_t daysOn):
    m_valveNumber{valveNumber}, m_hour{hour}, m_minute{minute}, m_timeCountdown{timeCountdown}, m_turnedOnTime{-1}, m_daysOn{daysOn}
{
    if(m_valveNumber < LOWEST_VALID_PIN_FOR_VALVE || m_valveNumber > HIGHEST_VALID_PIN_FOR_VALVE)
    {
        m_valveNumber = INVALID_VALVE;
        return;//pins less than 5 are used by arduino for bluetooth, interrupts, etc...
    }
    pinMode(m_valveNumber, OUTPUT);
    digitalWrite(m_valveNumber, HIGH);
}

bool Valve::isDayOn(int day) const
{
        if(day < 1 || day > 7)
        {
            return false;
        }
        return (m_daysOn >> day) & 0x1;
}

void Valve::setDayOn(int day, bool value)
{
        if(day < 1 || day > 7)
        {
            return;
        }
        uint8_t newbit = !!value;    // Also booleanize to force 0 or 1
        m_daysOn ^= (-newbit ^ m_daysOn) & (0x1 << day);
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
    int turnOffMinute = m_turnedOnTime + m_timeCountdown;
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
    return m_valveNumber;
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
    if(!isDayOn(dt.Dow))
    {
        return false;
    }
    int currentMinute = (dt.Dow - 1) * MINUTES_IN_DAY + dt.Hour * 60 + dt.Minute;
    int turnOnMinuteDay = m_hour * 60 + m_minute;
    
    int turnOnTime = (dt.Dow - 1) * MINUTES_IN_DAY + turnOnMinuteDay;
    int turnOffTime = turnOnTime + m_timeCountdown;
    if(turnOnTime - currentMinute <= 0 && turnOffTime - currentMinute > 0)
    {
        return true;
    }
    
    return false;
}

void Valve::turnOn(const DateTime& dt)
{
    m_turnedOnTime = ((dt.Dow-1) * MINUTES_IN_DAY) + m_hour*60 + m_minute;/*(dt.Hour*60) + dt.Minute;*/
#ifdef DEBUG
    Serial.print(F("setting turned on time: "));
    Serial.println(m_turnedOnTime);
#endif //DEBUG
    switchValve(HIGH);
}

void Valve::switchValve(int newHbridgeState)
{
    if(hBridgePin < 0)
    {
        return;
    }
    if(m_valveNumber == INVALID_VALVE)
    {
        return;
    }
    
    digitalWrite(hBridgePin, newHbridgeState);
    delay(RELAY_SETTLE_TIME);//wait for the relay to settle
    digitalWrite(m_valveNumber, LOW);//TODO: make sure that pin valvenumber is set to output
    delay(LATCH_TIME_MILLIS);
    digitalWrite(m_valveNumber, HIGH);//TODO: make sure that pin valvenumber is set to output
    digitalWrite(hBridgePin, HIGH); //TODO: find a way to restore the hBridge state
}

void Valve::setHBridgePin(int pinNum)
{
    hBridgePin = pinNum;
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
    serial.write(m_valveNumber);
    serial.write(m_hour);
    serial.write(m_minute);
    serial.write(m_daysOn);
    serial.write((uint8_t)(m_timeCountdown>>1*8));
    serial.write((uint8_t)(m_timeCountdown>>0*8));
}

bool Valve::isOn() const
{
    return m_turnedOnTime >= 0;
}
#ifdef DEBUG
String Valve::toString()
{
    static String nl = F("\n");
    String str = "";
    str += F("Valve Number: ");
    str += m_valveNumber;
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
    str += m_hour;
    str += nl;
    str += F("Minute: ");
    str += m_minute;
    str += nl;
    str += F("Time Countdown: ");
    str += m_timeCountdown;
    str += nl;
    str += F("Is On: ");
    str += ((m_turnedOnTime < 0) ? F("false"):F("true"));
    return str;
}

#endif //DEBUG
