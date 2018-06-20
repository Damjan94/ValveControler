#include <DS3231_Simple.h>
#include <Valve.h>


Valve valves[50];
DS3231_Simple myClock;
const int8_t H_BRIDGE_PIN[] = {3, 4};
const int BLUETOOTH_INTERRUPT_PIN = 13;
const int ALARM_INTERRUPT_PIN = 2;

const uint8_t SEND_VALVE = 0x1c;
const uint8_t RECEIVE_VALVE = 0x2c;

const uint8_t SEND_TIME = 0x1a;
const uint8_t RECEIVE_TIME = 0x2a;

const uint8_t SEND_TEMP_FLOAT = 0x1b;
const uint8_t SEND_TEMP = 0x2b;

const uint8_t SEND_HBRIDGE_PIN = 0x1d;

uint8_t valveCount;

String dateToString(const DateTime& dt);
void setup() 
{
    Serial.begin(38400);
    
    myClock.begin();
    
    pinMode(BLUETOOTH_INTERRUPT_PIN, INPUT);
    pinMode(ALARM_INTERRUPT_PIN, INPUT);

    //valve::setHbridgePin sets the pinmode of hbridge to output
    Valve::setHBridgePin(H_BRIDGE_PIN);
    Valve::switchHBridge(HIGH);
    valveCount = 0;
}

void loop() 
{
    uint8_t code = -1;
    if(Serial.available() > 0)
    {
      code = Serial.read();
    }
    
    switch(code)
    {
      case SEND_VALVE:
      {
        Serial.write(valveCount);
        for(size_t i = 0; i < valveCount; ++i)
        {
          valves[i].toSerial(Serial);
        }
        Serial.flush();
        break;
      }
      case RECEIVE_VALVE:
      {    
        uint8_t newValveCount = Serial.read();
        if(newValveCount > (sizeof(valves)/sizeof(Valve)))
        {
          //Serial.write(Message::TOO_MANY_VALVES);
          break;
        }
        //close the valves
        //before assigning the new valve count
        for(size_t i = 0; i < valveCount; ++i)
        {
            //turns off a valve only if the valve is currently on
            if(valves[i].isOn())//TODO tweak this, so it turns off every pin that a valve can be connected to
            {
                valves[i].turnOff();
                utility::delay(10);
            }
        }
        valveCount = newValveCount;
        for(size_t i = 0; i < valveCount; ++i)
        {
          int tryCount = 0;
          bool isValveInBuffer = true;
          while(Serial.available() < Valve::VALVE_NETWORK_SIZE)
          {
            utility::delay(100);
            ++tryCount;
            if(tryCount > 3)
            {
                while(Serial.available() > 0)
                {
                  Serial.read();//purge the stream
                }
                isValveInBuffer = false;
            }
          }
          if(!isValveInBuffer)
            break;
          valves[i].fromSerial(Serial);
          //Serial.write(Message::VALVE_RECEIVED);
        }
        break;
      }
      case SEND_TIME:
      {
        DateTime dt = myClock.read();
        dateTimeToSerial(dt, Serial);
        break;
      }
      case RECEIVE_TIME:
      {
        DateTime dt = dateTimeFromSerial(Serial);
        myClock.write(dt);
        break;
      }
      case SEND_TEMP_FLOAT:
      {
        float temp = myClock.getTemperatureFloat();
        uint8_t* byteTemp = (uint8_t*)&temp;
        for(int i = sizeof(temp)-1; i >= 0; --i)
        {
          Serial.write(byteTemp[i]);
        }
        break;
      }
      case SEND_TEMP:
      {
        uint8_t temp = myClock.getTemperature();
        Serial.write(temp);
        break;
      }
      case SEND_HBRIDGE_PIN:
      {
        int8_t* hbridgePin = Valve::getHBridgePin();
        Serial.write(hbridgePin[0]);
        Serial.write(hbridgePin[1]);
        break;
      }
    }
    
    const DateTime& dt = myClock.read();
    
    for(size_t i = 0; i < valveCount; ++i)
    {
      if(valves[i].checkTurnOn(dt))
      {
        valves[i].turnOn(dt);
      }
      if(valves[i].checkTurnOff(dt))
      {
        valves[i].turnOff();
      }
    }

    
    if(Valve::getHBridgeState() != HIGH)
    {
        Valve::switchHBridge(HIGH);
    }
    
    utility::delay(1000);
}

void dateTimeToSerial(const DateTime& dt, HardwareSerial& serial)
{
  serial.write(dt.Second);
  serial.write(dt.Minute);
  serial.write(dt.Hour);
  serial.write(dt.Dow);
  serial.write(dt.Day);
  serial.write(dt.Month);
  serial.write(dt.Year);
}

DateTime dateTimeFromSerial(HardwareSerial& serial)
{
  DateTime dt;
  dt.Second = serial.read();
  dt.Minute = serial.read();
  dt.Hour = serial.read();
  dt.Dow = serial.read();
  dt.Day = serial.read();
  dt.Month = serial.read();
  dt.Year = serial.read();
  //TODO: validate the time
  return dt;
}
#ifdef DEBUG
String dateToString(const DateTime& dt)
{
    static String nl = F("\n");
    String str = "";
    str += F("Day of week: ");
    str += dt.Dow;
    str += nl;
    
    str += F("Year: ");
    str += dt.Year;
    str += nl;
    
    str += F("Month: ");
    str += dt.Month;
    str += nl;
    
    str += F("Day of month: ");
    str += dt.Day;
    str += nl;
    
    str += F("Hour: ");
    str += dt.Hour;
    str += nl;
    
    str += F("Minute: ");
    str += dt.Minute;
    str += nl;
    
    str += F("Second: ");
    str += dt.Second;
    return str;
}
#endif //DEBUG


