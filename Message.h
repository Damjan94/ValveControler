#pragma once

#include <stdint.h>
#include <Arduino.h>
class Message
{
public:
	enum class Type : uint8_t
	{
		none	= 0x0,
		command = 0x1,  //Arduino is being commanded to do something(set valves, or time)
		request = 0x2,  //Arduino is being asked to provide the value (H bridge pins, temperature,...)
		info	= 0x3   //some info message
	};
	enum class Action : uint8_t
	{
		none				= 0x0,
		valve				= 0x1,
		time				= 0x2,
		temperature			= 0x3,
		temperatureFloat	= 0x4,
		hBridgePin			= 0x5,
		error				= 0x6,
		sleepTime			= 0x7
	};
	enum class Info : uint8_t
	{
		none			= 0x0,
		nominal			= 0x1,
		readyToSend		= 0x2,
		readyToReceive	= 0x3,
	};

	Message();
	Message(Type type, Action action, Info info, uint8_t size = 0, const void* bytes = nullptr);
	~Message();

	//returned if the operator[] index is out of bounds, or if m_data is not initilized
	//it is better to return false data, than to terminate the program.
	//TODO maybe make sure that m_data is always initilized?
	static uint8_t errorReturn;
	//returns m_data[i], if i < m_size, and m_data is not null
	//otherwise returns errorReturn.
	uint8_t& operator[](const size_t& i);
	const uint8_t& operator[](const size_t& i) const;

	Type		m_type;
	Action		m_action;
	Info		m_info;
	uint8_t		m_size;//in bytes
	uint8_t*	m_data;

	//TODO redesign, so that the header has it's own crc, and data should have it's own crc
	void send() const;
	void receive();


	const static uint8_t PROTOCOL_VERSION = 0;
	const static size_t NETWORK_SIZE = sizeof(PROTOCOL_VERSION) + sizeof(m_type) + sizeof(m_action) + sizeof(m_info) + sizeof(m_size);

};

