#include "Error.h"
#include "Utility.h"
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
	uint8_t returnCode = DSD_Clock.readLog(currentError.time, (uint8_t*)&currentError.number, sizeof(currentError.number));
	if (returnCode != 1)//1 means the function compleated succsesfully, 0 means that it failed
	{

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
/*
void Error::update(const DateTime& dt)
{
	currentTime = dt;
}
*/
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
	Message* msg = new Message(Message::Type::info, Message::Action::none, Message::Info::error, sizeof(Error::Description::number) + Utility::DATE_TIME_NETWORK_SIZE);
	
	(*msg)[0] = (uint8_t)Error::currentError.number;

	Utility::dateTimeToBytes(Error::currentError.time, *msg, 1);
}

void Error::clear()
{
	Error::currentError.number	= Error::Number::none;
	Error::currentError.time	= DateTime{ 0 };
}
