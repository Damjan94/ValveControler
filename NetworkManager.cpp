#include "NetworkManager.h"
#include <CRC32.h>
#include <DS3231_Simple.h>
#include "Utility.h"

NetworkManager::NetworkManager(DS3231_Simple& myClock, ValveController& myValveControler) :
	clock{ myClock }, valveControler{ myValveControler }, deviceSupportsUs{ false }
{

}


void NetworkManager::doHandshake(Message& msg)//TODO this wont work. make it so that the arduino initiates the handshake the first time
{
	deviceSupportsUs = false;

	Message msg;
	msg.type = Type::info;
	msg.info = Info::handshake;
	msg.send();

	uint8_t protocolVersion[2];
	Error::clear();
	size_t count = Utility::readBytes(sizeof(protocolVersion), protocolVersion);

	uint8_t minVersion, maxVersion;//the min and max versions. android should support everything in between, including min and max.
	minVersion = protocolVersion[0];
	maxVersion = protocolVersion[1];

	deviceSupportsUs = (!Error::hasError() && (Message::PROTOCOL_VERSION >= minVersion && Message::PROTOCOL_VERSION <= maxVersion) && count == 2);

	if (!deviceSupportsUs)
	{
		Error::log(Error::Number::handshakeFaliure);
	}
}

void NetworkManager::update(const DateTime& dt)
{
	if (!deviceSupportsUs)
		return;
    if(!Serial.available())
        return;
    //check the stream for data
	
	Message msg;
	msg.receive();
    switch(msg.type)
    {
        case Type::command:
        {
            switch(msg.action)
            {
                case Action::valve:
                {
                    valveControler.clear();
					Error::clear();
                    for(size_t i = 0; i < msg.itemCount; ++i)
                    {
                        Message msg;
                        msg.type = Type::info;
                        msg.info = Info::readyToReceive;
						msg.send();
						Valve v;
						Error::clear();
						receiveValve(v);
						if (!Error::hasError())
						{
							valveControler.addValve(v);
						}
						else
						{
							//TODO send the error code to android
							Error::clear();
						}
                    }
                    break;
                }
                case Action::time:
                {
					Error::clear();

					DateTime dt;
					receiveTime(dt);
					if (!Error::hasError())
					{
						clock.write(dt);
					}
					else
					{
						//TODO send the error
						Error::clear();
					}

					Error::clear();
                    break;
                }
                default:
                {
                    //TODO invalid Packet
                    break;
                }
            }
            break;
        }
        case Type::request:
        {
            switch(msg.action)
            {
                case Action::valve:
                {
                    Message msg;
                    msg.type = Type::command;
                    msg.action = Action::valve;
                    msg.itemCount = valveControler.getValveCount();
					msg.send();
                    //no space in ram to request a byte array from valve controller
                    for(size_t i = 0; i < valveControler.getValveCount(); ++i)
                    {
                        const Valve* v = valveControler.getValve(i);
                        if(v == nullptr)
                        {
                            //we are out of bounds
                            break;//TODO notify android that not all valves were sent?
                        }
                        sendValve(*v);
                        //arduino doesnt wait for the phone to send "ready to receive"
                    }
                    break;
                }
                case Action::time:
                {
                    sendTime(dt);
                    break;
                }
                case Action::temperature:
                {
                    uint8_t temp = clock.getTemperature();
                    sendTemperature(temp);
                    break;
                }
                case Action::temperatureFloat:
                {
                    float temp = clock.getTemperatureFloat();
                    sendTemperatureFloat(temp);
                    break;
                }
                case Action::hBridgePin:
                {
                    int8_t* hbridgePins = valveControler.getHBridgePin();
                    sendHBridgePin(hbridgePins);
                    break;
                }
                default:
                {
                    break;
                    //TODO invalid packet
                }
            }
            break;
        }
        case Type::info:
        {
			switch (msg.info)
			{
				default:
					break;
			}
            break;
        }
        default:
        {
            //TODO not a valid packet
            break;
        }
    }
}

void NetworkManager::receiveValve(Valve& v) const
{
	Error::clear();

    uint32_t crc32;
    Serial.readBytes((uint8_t*)(&crc32), sizeof(crc32));
    crc32 = ntohl(crc32);
    
    uint8_t valve[Valve::NETWORK_SIZE];
    
	Utility::readBytes(Valve::NETWORK_SIZE, valve);

    uint32_t calculatedCrc32 = CRC32::calculate(valve, Valve::NETWORK_SIZE);
    if(calculatedCrc32 != crc32)
    {
		Error::setError(Error::Number::invalidCrc);
    }

	if (Error::hasError())
		return;

	v.fromBytes(valve, true);
}

void NetworkManager::sendValve(const Valve& valve) const
{
    uint8_t* valveBytes = valve.toBytes(true);

    uint32_t crc32 = CRC32::calculate(valveBytes, Valve::NETWORK_SIZE);
    crc32 = htonl(crc32);
    Serial.write((uint8_t*)(&crc32), sizeof(crc32));
    Serial.write(valveBytes, Valve::NETWORK_SIZE);
    
    free(valveBytes);
    return;
}

void NetworkManager::receiveTime(DateTime& dt) const
{
	Error::clear();

    uint32_t crc32;
	Utility::readBytes(sizeof(crc32), (uint8_t*)(&crc32));
	if (Error::hasError())
		return;
    crc32 = ntohl(crc32);

    uint8_t dateTimeBytes[DATE_TIME_BYTE_COUNT];

    Utility::readBytes(DATE_TIME_BYTE_COUNT, dateTimeBytes);
	if (Error::hasError())
		return;
    uint32_t calculatedCrc32 = CRC32::calculate(dateTimeBytes, DATE_TIME_BYTE_COUNT);

    if(calculatedCrc32 != crc32)
    {
		Error::setError(Error::Number::invalidCrc);
        return;//TODO some error handeling would be nice... but I guess it's never gonna be year 0(this was written in 1019)
    }

	Utility::dateTimeFromBytes(dt, dateTimeBytes);
    //TODO: validate the time
}

void NetworkManager::sendTime(const DateTime& dt) const
{
    uint8_t dateTimeBytes[DATE_TIME_BYTE_COUNT];
	Utility::dateTimeToBytes(dt, dateTimeBytes);
    uint32_t crc32 = CRC32::calculate(dateTimeBytes, DATE_TIME_BYTE_COUNT);
    crc32 = htonl(crc32);
    Serial.write((uint8_t*)(&crc32), sizeof(crc32));
    Serial.write(dateTimeBytes, DATE_TIME_BYTE_COUNT);
}

void NetworkManager::sendTemperature(uint8_t temp) const
{
    uint32_t crc32 = CRC32::calculate(&temp, sizeof(temp));
    crc32 = htonl(crc32);
    Serial.write((uint8_t*)(&crc32), sizeof(crc32));
    Serial.write(temp);
}

void NetworkManager::sendTemperatureFloat(float temp) const
{
    uint32_t crc32 = CRC32::calculate(&temp, sizeof(temp));
    crc32 = htonl(crc32);
    Serial.write((uint8_t*)(&crc32), sizeof(crc32));
    temp = htonl((uint32_t)(temp));//TODO is it ok to change the endiannes of a float?
    Serial.write((uint8_t*)&temp, sizeof(temp));
}

void NetworkManager::sendHBridgePin(const int8_t* const hBridgePin) const
{
    uint32_t crc32 = CRC32::calculate(hBridgePin, sizeof(*hBridgePin)*2);//there are two hbridge pins.
    crc32 = htonl(crc32);
    Serial.write((uint8_t*)(&crc32), sizeof(crc32));
    Serial.write(hBridgePin[0]);
    Serial.write(hBridgePin[1]);
}



void NetworkManager::Message::receive()
{
	Error::clear();

	uint32_t crc32;
	Utility::readBytes(sizeof(crc32), (uint8_t*)(&crc32));

	if (Error::hasError())
		return;

	crc32 = ntohl(crc32);

	uint8_t buffer[NetworkManager::Message::NETWORK_SIZE];

	Utility::readBytes(NetworkManager::Message::NETWORK_SIZE, buffer);
	if (Error::hasError())
		return;

	uint32_t calculatedCrc32 = CRC32::calculate(buffer, NetworkManager::Message::NETWORK_SIZE);
	if (crc32 != calculatedCrc32)
	{
		Error::setError(Error::Number::invalidCrc);
		return;
	}

	//after the crc check, we know that the recived message is valid
	if (buffer[0] != Message::PROTOCOL_VERSION)
	{
		Error::setError(Error::Number::invalidMessageProtocolVersion);
		return;
	}

	//TODO do proper conversions from byte to class enum. or just use enums...
	
	this->type = static_cast<Type>(buffer[1]);
	this->action = static_cast<Action>(buffer[2]);
	this->info = static_cast<Info>(buffer[3]);
	this->errorNo = static_cast<Error::Number>(buffer[4]);
	this->itemCount = buffer[5];
}

void NetworkManager::Message::send() const
{
    uint8_t buffer[Message::NETWORK_SIZE];//1 byte for message protocol version, 1 byte for Type, 1 for Action, and 1 for Info; 1 for item count
	buffer[0] = Message::PROTOCOL_VERSION;
    buffer[1] = static_cast<uint8_t>(this->type);
    buffer[2] = static_cast<uint8_t>(this->action);
    buffer[3] = static_cast<uint8_t>(this->info);
	buffer[4] = static_cast<uint8_t>(this->errorNo);
    buffer[5] = this->size;//TODO maybe make a message contain the pointer to the data we need t osend, and item count could be byte count
    uint32_t crc32 = CRC32::calculate(buffer, sizeof(buffer));
    crc32 = htonl(crc32);
    Serial.write((uint8_t*)(&crc32), sizeof(crc32));
    Serial.write(buffer, sizeof(buffer));
	Serial.write(data, this->size);
}
