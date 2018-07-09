#include <LowPower.h>
#include <DS3231_Simple.h>
#include <ValveController.h>

DS3231_Simple myClock;
ValveController myValveController;

const int BLUETOOTH_INTERRUPT_PIN = 3;
const int ALARM_INTERRUPT_PIN = 2;

const uint8_t SEND_VALVE = 0x1c;
const uint8_t RECEIVE_VALVE = 0x2c;

const uint8_t SEND_TIME = 0x1a;
const uint8_t RECEIVE_TIME = 0x2a;

const uint8_t SEND_TEMP_FLOAT = 0x1b;
const uint8_t SEND_TEMP = 0x2b;

const uint8_t SEND_HBRIDGE_PIN = 0x1d;

volatile bool isBluetoothConnected;
volatile bool isHandshakeSuccessful;

volatile bool isAlarmActive;

#ifdef DEBUG
String dateToString(const DateTime& dt);
#endif //DEBUG

void bluetoothISR()
{
    isBluetoothConnected = !isBluetoothConnected;
    isHandshakeSuccessful = false;
}

void alarmISR()
{
    isAlarmActive = !isAlarmActive;
}

void clearStream(HardwareSerial& stream)
{
  while(stream.available()) stream.read();
}
void setup() 
{
    Serial.begin(38400);
    
    myClock.begin();
    
    pinMode(BLUETOOTH_INTERRUPT_PIN, INPUT);
    pinMode(ALARM_INTERRUPT_PIN, INPUT);
    
    isBluetoothConnected = digitalRead(BLUETOOTH_INTERRUPT_PIN) == HIGH;
    isHandshakeSuccessful = false;
    
    isAlarmActive = digitalRead(ALARM_INTERRUPT_PIN) == LOW; //alarm interrupt is active low
    
    attachInterrupt(digitalPinToInterrupt(BLUETOOTH_INTERRUPT_PIN), bluetoothISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ALARM_INTERRUPT_PIN), alarmISR, CHANGE);
}

void checkStream()
{
    if(Serial.available() < 1) return;
    uint8_t networkCode = Serial.read();
    
    switch(networkCode)
    {
      case SEND_VALVE:
      {
        myValveController.sendValves(Serial);
        break;
      }
      case RECEIVE_VALVE:
      {    
        myValveController.receiveValves(Serial);
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
        int8_t* hbridgePin = myValveController.getHBridgePin();
        Serial.write(hbridgePin[0]);
        Serial.write(hbridgePin[1]);
        break;
      }
    }
}

bool doHandshake()
{
  clearStream(Serial);
  Serial.write(0x5);
  if(Serial.read()!=0x5) return false;
  Serial.write(0xF);
  if(Serial.read()!=0xF) return false;
  return true;
}
void loop() 
{
    
    if(isBluetoothConnected)
    {
        if(isHandshakeSuccessful)
        {
            checkStream();
        }
        else if(true)
        {
            isHandshakeSuccessful = doHandshake();
        }
        else
        {
          clearStream(Serial);
        }
    }
    
    const DateTime& dt = myClock.read();
    
    myValveController.updateValves(dt);
    
    if(!isBluetoothConnected && !isAlarmActive)
    {
        DateTime alarmDate = myValveController.getSoonestActionDate(dt);
        if(myClock.compareTimestamps(alarmDate, dt) > 0 )
        {
            myClock.disableAlarms();
            myClock.setAlarm(alarmDate, DS3231_Simple::ALARM_MATCH_MINUTE_HOUR_DATE);
            LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
        }
    }
    utility::delay(500);
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


