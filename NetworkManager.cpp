#include "NetworkManager.h"
#include <CRC32.h>
#include <util.h>   //for ntohx and htonx
#include <DS3231_Simple.h>

NetworkManager::NetworkManager(DS3231_Simple& myClock, ValveController& myValveControler) : m_clock{myClock}, m_valveControler{ myValveControler }
{

}
void NetworkManager::update()
{
    if(!Serial.available())
        return;
    //check the stream for data
	
	Message msg = receiveMessage();

    switch(msg.type)
    {
        case Type::command:
        {
            switch(msg.action)
            {
                case Action::valve:
                {
                    m_valveControler.clear();
                    for(size_t i = 0; i < msg.itemCount; ++i)
                    {
                        Message msg;
                        msg.type = Type::info;
                        msg.info = Info::readyToReceive;
                        sendMessage(msg);
						m_valveControler.addValve(receiveValve());
                    }
                    break;
                }
                case Action::time:
                {
					DateTime dt = receiveTime();
					if(dt.Year != 0)//I am going on an assumption that it will never be year 0 :P
						m_clock.write(dt);
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
                    msg.itemCount = m_valveControler.getValveCount();
                    sendMessage(msg);
                    //no space in ram to request a byte array from valve controller
                    for(size_t i = 0; i < m_valveControler.getValveCount(); ++i)
                    {
                        const Valve* v = m_valveControler.getValve(i);
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
                    sendTime(m_clock.read());
                    break;
                }
                case Action::temperature:
                {
                    uint8_t temp = m_clock.getTemperature();
                    sendTemperature(temp);
                    break;
                }
                case Action::temperatureFloat:
                {
                    float temp = m_clock.getTemperatureFloat();
                    sendTemperatureFloat(temp);
                    break;
                }
                case Action::hBridgePin:
                {
                    int8_t* hbridgePins = m_valveControler.getHBridgePin();
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

            break;
        }
        default:
        {
            //TODO not a valid packet
            break;
        }
    }
}

Valve NetworkManager::receiveValve() const
{
    
    uint32_t crc32;
    Serial.readBytes((uint8_t*)(&crc32), sizeof(crc32));
    crc32 = ntohl(crc32);
    
    uint8_t valve[Valve::NETWORK_SIZE];
    
    readBytes(Valve::NETWORK_SIZE, valve);

    uint32_t calculatedCrc32 = CRC32::calculate(valve, Valve::NETWORK_SIZE);
    if(calculatedCrc32 != crc32)
    {
        return Valve();//TODO how do I throw an exception....
    }

    return Valve(valve, true);
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

DateTime NetworkManager::receiveTime() const
{
    uint32_t crc32;
    Serial.readBytes((uint8_t*)(&crc32), sizeof(crc32));
    crc32 = ntohl(crc32);

    uint8_t dateTimeBytes[DATE_TIME_BYTE_COUNT];

    readBytes(DATE_TIME_BYTE_COUNT, dateTimeBytes);
    uint32_t calculatedCrc32 = CRC32::calculate(dateTimeBytes, DATE_TIME_BYTE_COUNT);
    DateTime dt{};
    if(calculatedCrc32 != crc32)
    {
        return dt;//TODO some error handeling would be nice... but I guess it's never gonna be year 0(this was written in 1019)
    }

    dt.Second   = dateTimeBytes[0];
    dt.Minute   = dateTimeBytes[1];  
    dt.Hour     = dateTimeBytes[2];
    dt.Dow      = dateTimeBytes[3];
    dt.Day      = dateTimeBytes[4];
    dt.Month    = dateTimeBytes[5];
    dt.Year     = dateTimeBytes[6];
    //TODO: validate the time
    return dt;
}

void NetworkManager::sendTime(const DateTime& dt) const
{
    uint8_t dateTimeBytes[DATE_TIME_BYTE_COUNT];
    dateTimeBytes[0] = dt.Second;
    dateTimeBytes[1] = dt.Minute;
    dateTimeBytes[2] = dt.Hour;
    dateTimeBytes[3] = dt.Dow;
    dateTimeBytes[4] = dt.Day;
    dateTimeBytes[5] = dt.Month;
    dateTimeBytes[6] = dt.Year;
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

size_t NetworkManager::readBytes(size_t count, uint8_t* data) const
{
    size_t readBytes = 0;
    size_t iterationCount = 1;
    static const size_t MAX_ITERATION_COUNT = 3;
    while(readBytes < count)
    {
        readBytes += Serial.readBytes(data+readBytes, count-readBytes);
        if(iterationCount >= MAX_ITERATION_COUNT && readBytes < count)
        {
            break;
        }
        ++iterationCount;
        //TODO maybe sleep for a while?
    }
    return readBytes;
}

NetworkManager::Message NetworkManager::receiveMessage() const
{
	uint32_t crc32;
	size_t count = Serial.readBytes((uint8_t*)(&crc32), sizeof(crc32));
	if (count != sizeof(crc32))
		return;
	crc32 = ntohl(crc32);

	uint8_t buffer[NetworkManager::Message::NETWORK_SIZE];

	//TODO this should done on every readBytes call, but I am too lazy...
	count = readBytes(NetworkManager::Message::NETWORK_SIZE, buffer);
	if (count != NetworkManager::Message::NETWORK_SIZE)
		return;

	uint32_t calculatedCrc32 = CRC32::calculate(buffer, NetworkManager::Message::NETWORK_SIZE);
	Message msg;
	if (crc32 != calculatedCrc32)
	{
		return msg;
	}

	//TODO do proper conversions from byte to class enum. or just use enums...
	
	msg.type = static_cast<Type>(buffer[0]);
	msg.action = static_cast<Action>(buffer[1]);
	msg.info = static_cast<Info>(buffer[2]);
	msg.itemCount = buffer[3];
	return msg;
}

void NetworkManager::sendMessage(const Message& msg) const
{
    uint8_t buffer[4];//1 byte for Type, 1 for Action, and 1 for Info; 1 for item count
    buffer[0] = static_cast<uint8_t>(msg.type);
    buffer[1] = static_cast<uint8_t>(msg.action);
    buffer[2] = static_cast<uint8_t>(msg.info);
    buffer[3] = msg.itemCount;
    uint32_t crc32 = CRC32::calculate(buffer, sizeof(buffer));
    crc32 = htonl(crc32);
    Serial.write((uint8_t*)(&crc32), sizeof(crc32));
    Serial.write(buffer, sizeof(buffer));
}
