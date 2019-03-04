#pragma once
#include <DS3231_Simple.h>

class Error
{
public:
	Error();
	~Error();

	enum class Number : uint8_t
	{
		none							= 0x0,
		handshakeFaliure				= 0x2,
		invalidCrc						= 0x3,
		couldNotReadAllBytes			= 0x4,
		invalidMessageProtocolVersion	= 0x5
	};
	struct Description
	{
		Number number;
		DateTime time;
	};

	static bool hasError();
	static Error::Description getError();
	static void setError(Error::Description num);

	static void log();
	static void log(Error::Description error);
	/*
	gets the oldest log, and deletes it from storage.
	@return returns the error number that was logged.
	returns Error::Number::none, if there are no stored logs
	*/
	static void loadNextLogged();
	static void toMessage(NetworkManager::Message& msg);
	//resets current error to Error::Number::none
	static void clear();

protected:
	static Description currentError;
};

