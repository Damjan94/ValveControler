#include <CRC32.h>
#include <LowPower.h>
#include <DS3231_Simple.h>
#include "ValveControler.h"
#include "NetworkManager.h"
#include "Error.h"
#include "Utility.h"

DS3231_Simple   myClock;
ValveController myValveController;
NetworkManager  myNetworkManager(myClock, myValveController);

DS3231_Simple& Error::DSD_Clock = myClock;

const int BLUETOOTH_INTERRUPT_PIN = 3;
const int ALARM_INTERRUPT_PIN = 2;

volatile bool isBluetoothConnected;
volatile uint8_t connectedDeviceErrorCount;

bool ignoreTheDevice;

volatile bool isAlarmActive;

#ifdef DEBUG
bool alter = false;
void alternate();
String dateToString(const DateTime& dt);
#endif //DEBUG

void bluetoothISR()
{
    isBluetoothConnected = !isBluetoothConnected;
	ignoreTheDevice = false;
	connectedDeviceErrorCount = 0;
}

void alarmISR()
{
    isAlarmActive = !isAlarmActive;
}

void setup() 
{
    Serial.begin(38400);
    
    myClock.begin();
    
    pinMode(BLUETOOTH_INTERRUPT_PIN, INPUT);
    pinMode(ALARM_INTERRUPT_PIN, INPUT);

    pinMode(0, INPUT);
    pinMode(1, INPUT);
    
	isBluetoothConnected = digitalRead(BLUETOOTH_INTERRUPT_PIN) == HIGH;
	connectedDeviceErrorCount = 0;

	ignoreTheDevice = false;

    isAlarmActive = digitalRead(ALARM_INTERRUPT_PIN) == LOW; //alarm interrupt is active low
    
    attachInterrupt(digitalPinToInterrupt(BLUETOOTH_INTERRUPT_PIN), bluetoothISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ALARM_INTERRUPT_PIN), alarmISR, CHANGE);

	//the delys are here in case arduino has not finished setting up yet...
	Utility::delay(1000);
	Utility::delay(100);
}

void loop()
{
	const DateTime& dt = myClock.read();

	Error::clear();
	Error::setCurrentTime(dt);

    if(isBluetoothConnected)
    {
		//if we are not ignoring the device, try to read any sent bytes, otherwise, empty the buffer
		if (!ignoreTheDevice)
		{
			myNetworkManager.update(dt);
		}
		else
		{
			while (Serial.available())
			{
				Serial.read();
			}
		}
    }

	if (Error::hasError())
	{
		Error::log();
		if (isBluetoothConnected)
		{
			Message* errorMsg = Error::toMessage();
			errorMsg->send();
			delete errorMsg;
		}
		Error::clear();
		++connectedDeviceErrorCount;
		if (connectedDeviceErrorCount > 3)
		{
			ignoreTheDevice = true;
		}
	}

    myValveController.update(dt);
	
	//go to sleep logic
    if(!isBluetoothConnected && !isAlarmActive)
    {
		
        DateTime alarmDate = myValveController.getSoonestActionDate(dt);
        if(myClock.compareTimestamps(alarmDate, dt) > 0 )
        {
			myNetworkManager.setLastLongSleep(dt);
            myClock.disableAlarms();
            myClock.setAlarm(alarmDate, DS3231_Simple::ALARM_MATCH_MINUTE_HOUR_DATE);
            LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);//FIXME apparently the arduino isn't sleeping
        }
		
    }
	myNetworkManager.setLastShortSleep(dt);
    LowPower.idle(SLEEP_120MS, ADC_ON, TIMER2_ON, TIMER1_ON, TIMER0_ON, SPI_ON, USART0_ON, TWI_ON);
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

void alternate()
{
	Utility::delay(1000);
	if (alter == true)
	{
		digitalWrite(4, HIGH);
		digitalWrite(13, LOW);
		alter = false;
	}
	else
	{
		digitalWrite(4, LOW);
		digitalWrite(13, HIGH);
		alter = true;
	}
	Utility::delay(1000);
}
#endif //DEBUG


