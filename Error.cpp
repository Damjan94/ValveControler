#include "Error.h"

Error::Description Error::currentError{ Error::Number::none, DS3231_Simple::DateTime{0} };

bool Error::hasError()
{
	return currentError.number != Error::Number::none;
}

Error::Description Error::getError()
{
	return currentError;
}

void Error::loadNextLogged()
{
	if (hasError())
	{
		log();//this works coz read log reads the oldest log...
		clear();
	}
	uint8_t returnCode = myClock.readLog(currentError.time, currentError.number);
	if (returnCode != 1)//1 means the function compleated succsesfully, 0 means that it failed
	{

	}
}

void Error::setError(Error::Description err)
{
	currentError = err;
}

void Error::log()
{
	Error::log(currentError);
}

void Error::log(Error::Description err)
{
	myClock.writeLog(err.time, (uint8_t)err.number);
	Error::clear();
}

void Error::toMessage(NetworkManager::Message& msg)
{
	msg.info = NetworkManager::Info::error;
	msg.error = Error::currentError;

	Utility::dateTimeToBytes
}

void Error::clear()
{
	Error::currentError.number	= Error::Number::none;
	Error::currentError.time	= DateTime{ 0 };
}
