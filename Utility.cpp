#include "Utility.h"
#include "Error.h"
#include <time.h>
#include <Arduino.h>

size_t Utility::readBytes(size_t count, uint8_t* data)
{
	Error::clear();

	size_t readBytes = 0;
	size_t iterationCount = 1;
	static const size_t MAX_ITERATION_COUNT = 3;
	while (readBytes < count)
	{
		readBytes += Serial.readBytes(data + readBytes, count - readBytes);
		if (iterationCount >= MAX_ITERATION_COUNT && readBytes < count)
		{
			Error::setError(Error::Number::couldNotReadAllBytes);
			break;
		}
		++iterationCount;

		if (readBytes < count)
			delay(DEFAULT_NETWORK_WAIT_TIME);
	}
	return readBytes;
}

void Utility::delay(unsigned long ms)
{
	unsigned long startTime = millis();
	while ((millis() - startTime) < ms);
}

int Utility::dateTimeToMinutesInWeek(const DateTime& dt)
{
	static const int MINUTES_IN_DAY = 1440;
	return ((dt.Dow - 1) * MINUTES_IN_DAY) + (dt.Hour * 60) + dt.Minute;
}

DateTime Utility::addMinutesToDate(int minutes, const DateTime& date)
{
	struct tm timeStructure;
	timeStructure.tm_sec = date.Second;
	timeStructure.tm_min = date.Minute;
	timeStructure.tm_hour = date.Hour;
	timeStructure.tm_mday = date.Day;
	timeStructure.tm_mon = date.Month - 1;
	timeStructure.tm_year = date.Year + 100;
	timeStructure.tm_wday = date.Dow - 1;
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
	newTime.Month = tmStruct.tm_mon + 1;
	newTime.Year = tmStruct.tm_year - 100;
	newTime.Dow = tmStruct.tm_wday + 1;

	return newTime;
}

void Utility::dateTimeFromBytes(DateTime& dt, const Message& dateTimeBytes, int offset)
{
	dt.Second	= dateTimeBytes[offset + 0];
	dt.Minute	= dateTimeBytes[offset + 1];
	dt.Hour		= dateTimeBytes[offset + 2];
	dt.Dow		= dateTimeBytes[offset + 3];
	dt.Day		= dateTimeBytes[offset + 4];
	dt.Month	= dateTimeBytes[offset + 5];
	dt.Year		= dateTimeBytes[offset + 6];
}

void Utility::dateTimeToBytes(const DateTime& dt, Message& dateTimeBytes, int offset)
{
	dateTimeBytes[offset + 0] = dt.Second;
	dateTimeBytes[offset + 1] = dt.Minute;
	dateTimeBytes[offset + 2] = dt.Hour;
	dateTimeBytes[offset + 3] = dt.Dow;
	dateTimeBytes[offset + 4] = dt.Day;
	dateTimeBytes[offset + 5] = dt.Month;
	dateTimeBytes[offset + 6] = dt.Year;
}

