#include "Message.h"
#include "Utility.h"
#include "Error.h"
#include "CRC32.h"

uint8_t Message::errorReturn = 0x0;

Message::Message() : Message(Type::none, Action::none, Info::none)
{

}

Message::Message(Type type, Action action, Info info, uint8_t size, const void* bytes) : m_type{ type }, m_action{ action }, m_info{ info }, m_size{ size }, m_data{nullptr}
{
	if (size > 0)
	{
		m_data = new uint8_t[size];
	}
	if (bytes != nullptr && m_data != nullptr)
	{
		memcpy(m_data, bytes, size);
	}
}

Message::~Message()
{
	delete[] m_data;
}

uint8_t& Message::operator[](const size_t& i)
{
	if (m_data != nullptr && i < m_size)
		return m_data[i];
	return errorReturn;
}

const uint8_t& Message::operator[](const size_t& i) const
{
	return m_data[i];
}

void Message::send() const
{
	uint8_t rawBytes[NETWORK_SIZE];
	rawBytes[0] = Message::PROTOCOL_VERSION;
	rawBytes[1] = static_cast<uint8_t>(this->m_type);
	rawBytes[2] = static_cast<uint8_t>(this->m_action);
	rawBytes[3] = static_cast<uint8_t>(this->m_info);
	rawBytes[4] = this->m_size;

	uint32_t crc32 = CRC32::calculate(rawBytes, NETWORK_SIZE);
	crc32 = htonl(crc32);

	uint32_t dataCrc = 0;
	if (m_size > 0)
	{
		dataCrc = CRC32::calculate(this->m_data, this->m_size);
		dataCrc = htonl(dataCrc);
	}
	Serial.write((uint8_t*)(&crc32), sizeof(crc32));
	Serial.write(rawBytes, sizeof(rawBytes));
	if (m_size > 0)
	{
		Serial.write((uint8_t*)(&dataCrc), sizeof(dataCrc));
		Serial.write(this->m_data, this->m_size);
	}
}

void Message::receive()
{
	uint32_t receivedCrc32;
	Utility::readBytes(sizeof(receivedCrc32), (uint8_t*)(&receivedCrc32));
	if (Error::hasError())
		return;
	receivedCrc32 = ntohl(receivedCrc32);

	uint8_t rawBytes[NETWORK_SIZE];

	Utility::readBytes(NETWORK_SIZE, rawBytes);
	if (Error::hasError())
		return;

	if (CRC32::calculate(rawBytes, NETWORK_SIZE) != receivedCrc32)
	{
		Error::setError(Error::Number::invalidCrc);
		return;
	}

	if (rawBytes[0] != Message::PROTOCOL_VERSION)
	{
		Error::setError(Error::Number::invalidMessageProtocol);
		return;
	}

	this->m_type = static_cast<Type>(rawBytes[1]);
	this->m_action = static_cast<Action>(rawBytes[2]);
	this->m_info = static_cast<Info>(rawBytes[3]);
	this->m_size = rawBytes[4];

	delete[] m_data;
	m_data = nullptr;

	if (m_size > 0)
	{
		m_data = new uint8_t[m_size];
		uint32_t dataCrc;
		Utility::readBytes(sizeof(dataCrc), (uint8_t*)&dataCrc);
		if (Error::hasError())
			return;
		dataCrc = ntohl(dataCrc);

		Utility::readBytes(m_size, m_data);
		if (Error::hasError())
			return;

		if (CRC32::calculate(m_data, m_size) != dataCrc)
		{
			Error::setError(Error::Number::invalidCrc);
			return;
		}
	}
}