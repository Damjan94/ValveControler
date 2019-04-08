#include "Error.h"
#include "Utility.h"
Error::Description Error::currentError{ Error::Number::none, DS3231_Simple::DateTime{0, 0, 0, 0, 0, 0, 0} };
DateTime Error::currentTime{ 0, 0, 0, 0, 0, 0, 0};

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
	uint8_t returnCode = DSD_Clock.readLog(currentError.time, (uint8_t*)&currentError.number, sizeof(currentError.number));
	if (returnCode != 1)//1 means the function compleated succsesfully, 0 means that it failed
	{
		memset(&currentError, 0, sizeof(currentError));
		currentError.number = Error::Number::none;//just in case we later change none to be something other than 0(used in hasError() function)... 
	}
}

void Error::setError(Error::Number num)
{
	setError(Error::Description{ num, currentTime });
}

void Error::setError(Error::Description err)
{
	currentError = err;
}

void Error::setCurrentTime(const DateTime & dt)
{
	currentTime = dt;
}

void Error::log()
{
	Error::log(currentError);
}

void Error::log(Error::Number num)
{
	Error::log(Error::Description{ num, currentTime });
}

void Error::log(Error::Description err)
{
	DSD_Clock.writeLog(err.time, err.number);
	Error::clear();
}

Message* Error::toMessage()
{
	Message* msg = new Message(Message::Type::command, Message::Action::error, Message::Info::none, sizeof(Error::Description::number) + Utility::DATE_TIME_NETWORK_SIZE);
	
	(*msg)[0] = (uint8_t)Error::currentError.number;

	Utility::dateTimeToBytes(Error::currentError.time, *msg, 1);
	return msg;
}

void Error::clear()
{
	Error::currentError.number	= Error::Number::none;
	Error::currentError.time	= DateTime{0, 0, 0, 0, 0, 0, 0};
}
