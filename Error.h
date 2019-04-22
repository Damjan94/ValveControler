#pragma once
#include <DS3231_Simple.h>
#include "Message.h"

class Error
{
public:
	enum class Number : uint8_t
	{
		none							= 0x0,
		startedIgnoringTheDevice		= 0x1,
		invalidCrc						= 0x2,
		couldNotReadAllBytes			= 0x3,
		tooManyValvesToReceive			= 0x4,
		invalidMessageProtocol			= 0xFF
	};
	struct Description
	{
		Number number;
		DateTime time;
	};

	static DS3231_Simple& DSD_Clock;

	static bool hasError();
	static Error::Description getError();

	static void setError(Error::Number num);
	static void setError(Error::Description num);

	static void setCurrentTime(const DateTime& dt);

	static void log();
	static void log(Error::Number num);
	static void log(Error::Description error);
	/*
	gets the oldest log, and deletes it from storage.
	@return returns the error number that was logged.
		or Error::Number::none, if there are no stored logs
	*/
	static void loadNextLogged();

	static Message* toMessage();
	//resets current error to Error::Number::none
	static void clear();

protected:
	static Description currentError;
	static DateTime currentTime;
};

