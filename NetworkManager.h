#pragma once

#include <stdint.h>
#include <stdio.h>
#include <DS3231_Simple.h>
#include "ValveControler.h"

class NetworkManager
{
    public:
    enum class Type : uint8_t
    {
        none    = 0x0,
        command = 0x1,  //Arduino is being commanded to do something(set valves, or time)
        request = 0x2,  //Arduino is being asked to provide the value (H bridge pins, temperature,...)
        info    = 0x3   //some info message
    };
    enum class Action : uint8_t
    {
        none                = 0x0,
        valve               = 0x1,
        time                = 0x2,
        temperature         = 0x3,
        temperatureFloat    = 0x4,
        hBridgePin          = 0x5,
    };
	enum class Info : uint8_t
	{
		none = 0x0,
		nominal = 0x1,
		error = 0x2,
		readyToSend = 0x3,
		readyToReceive = 0x4,
		handshake = 0x5
    };

    struct Message
    {
		const static size_t NETWORK_SIZE = 6;
		const static uint8_t PROTOCOL_VERSION = 0;
        Type type;
        Action action;
        Info info;
        uint8_t size;//sometimes this is how many bytes there are, and sometimes it means how many items of specified type there are
		uint8_t* data;
        Message():type(Type::none), action(Action::none), info(Info::none), size(0){}
		//Message() :type(Type::none), action(Action::none), info(Info::none), itemCount(0) {}

		void send() const;
		void receive();
    };

    const static size_t DATE_TIME_BYTE_COUNT = sizeof(DateTime::Second) + sizeof(DateTime::Minute) + sizeof(DateTime::Hour) + sizeof(DateTime::Dow) + sizeof(DateTime::Day) + sizeof(DateTime::Month) + sizeof(DateTime::Year);
    static_assert(DATE_TIME_BYTE_COUNT != 6, "DateTime not expected size of 6");

    NetworkManager(DS3231_Simple& myClock, ValveController& myController);

    void update(const DateTime& dt);
	void doHandshake(Message& msg);

    protected:
    void receiveValve(Valve&) const;
    void sendValve(const Valve&) const;

    void receiveTime(DateTime&) const;
    void sendTime(const DateTime& dt) const;

    void sendTemperature(uint8_t temperature) const;
    void sendTemperatureFloat(float temperature) const;

    void sendHBridgePin(const int8_t* const pins) const;

    private:

    DS3231_Simple&		clock;
	ValveController&	valveControler;
	bool				deviceSupportsUs;
};
