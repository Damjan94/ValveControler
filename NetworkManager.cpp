#include "NetworkManager.h"
#include <CRC32.h>
#include <DS3231_Simple.h>
#include "Utility.h"
#include "Error.h"

NetworkManager::NetworkManager(DS3231_Simple& myClock, ValveController& myValveControler) :
	clock{ myClock }, valveControler{ myValveControler }
{

}

void NetworkManager::update(const DateTime& dt)
{
    if(Serial.available() < Message::NETWORK_SIZE)
        return;
	
	Error::clear();

	Message msg;
	msg.receive();

	if (Error::hasError())
		return;

    switch(msg.m_type)
    {
		case Message::Type::command:
        {
            switch(msg.m_action)
            {
                case Message::Action::valve:
                {
					if (msg[0] > ValveController::MAX_VALVES - 1)
					{
						Error::setError(Error::Number::tooManyValvesToReceive);
						Message* msg = Error::toMessage();
						msg->send();
						delete msg;
						Error::clear();
						break;
					}

					valveControler.clear();
                    for(size_t i = 0; i < msg[0]; ++i)
                    {
                        Message msg(Message::Type::info, Message::Action::none, Message::Info::readyToReceive);
						msg.send();
						msg.receive();
						if (Error::hasError())
							break;//FIXME without clearing the remainder of the buffer, the input stream has garbage data, and is useless
						Valve v;
						v.fromMessage(msg);
						valveControler.addValve(v);
                    }
                    break;
                }
                case Message::Action::time:
                {
					DateTime dt;
					Utility::dateTimeFromBytes(dt, msg);
					if (Error::hasError())
					{
						Message* errorMsg = Error::toMessage();
						errorMsg->send();
						delete errorMsg;
						Error::clear();
						break;
					}
					//TODO: validate the time
					clock.write(dt);
                    break;
                }
                default:
                {
					logInvalidPacket();
                    break;
                }
            }
            break;
        }
        case Message::Type::request:
        {
            switch(msg.m_action)
            {
                case Message::Action::valve:
                {
					//I put this in different scope, so I can reuse the name msg (bad practice?)
					{
						Message msg(Message::Type::command, Message::Action::valve, Message::Info::none, 1);
						msg[0] = (uint8_t)valveControler.getValveCount();
						msg.send();
					}
                    //no space in ram to request a byte array from valve controller
                    for(size_t i = 0; i < valveControler.getValveCount(); ++i)
                    {
                        const Valve* v = valveControler.getValve(i);
                        if(v == nullptr)
                        {
                            //we are out of bounds
                            break;//TODO notify android that not all valves were sent?
                        }
						Message* valveMsg = v->toMessage();
						valveMsg->send();
						delete valveMsg;
                        //arduino doesn't wait for the phone to send "ready to receive"
                    }
                    break;
                }
                case Message::Action::time:
                {
					Message dateMsg(Message::Type::command, Message::Action::time, Message::Info::none, Utility::DATE_TIME_NETWORK_SIZE);
					Utility::dateTimeToBytes(dt, dateMsg);
					dateMsg.send();
                    break;
                }
                case Message::Action::temperature:
                {
                    uint8_t temp = clock.getTemperature();
					Message temperatureMsg(Message::Type::command, Message::Action::temperature, Message::Info::none, sizeof(temp), &temp);
					temperatureMsg.send();
                    break;
                }
                case Message::Action::temperatureFloat:
                {
                    float temp = clock.getTemperatureFloat();
					const static size_t SPACE_FOR_CHAR = 10; //TODO figure out exactly how much space I need...
					char temperatureChar[SPACE_FOR_CHAR];
					memset(temperatureChar, 0, SPACE_FOR_CHAR);
					dtostrf(temp, 2, 2, temperatureChar);//FIXME dtostrf uses too much memorry(so, I've heard)
					Message temperatureMsg(Message::Type::command, Message::Action::temperatureFloat, Message::Info::none, SPACE_FOR_CHAR, temperatureChar);
					temperatureMsg.send();
                    break;
                }
                case Message::Action::hBridgePin:
                {
                    int8_t* hbridgePins = valveControler.getHBridgePin();//this should return 2 hbridge pins
					Message hBridgeMsg(Message::Type::command, Message::Action::hBridgePin, Message::Info::none, sizeof(hbridgePins[0] * 2), hbridgePins);
					hBridgeMsg.send();
                    break;
                }
				case Message::Action::error:
				{
					do
					{
						Error::loadNextLogged();
						Message* msg = Error::toMessage();
						msg->send();
						delete msg;
					} while (!Error::hasError());
					Message doneSendingError(Message::Type::info, Message::Action::error, Message::Info::nominal);
					doneSendingError.send();
					break;
				}
				case Message::Action::sleepTime:
				{
					Message sleepMessage(Message::Type::command, Message::Action::sleepTime, Message::Info::none, Utility::DATE_TIME_NETWORK_SIZE*2);
					Utility::dateTimeToBytes(m_shortSleepTime, sleepMessage, 0);
					Utility::dateTimeToBytes(m_longSleepTime, sleepMessage, Utility::DATE_TIME_NETWORK_SIZE);
					sleepMessage.send();
					break;
				}
                default:
                {
                    break;
					logInvalidPacket();
                }
            }
            break;
        }
        case Message::Type::info:
        {
			switch (msg.m_info)
			{
				default:
				{
					logInvalidPacket();
					break;
				}
			}
            break;
        }
		case Message::Type::none:
		{
			if (msg.m_action == Message::Action::none && msg.m_info == Message::Info::none)
			{
				msg.send();//echo back our protocol version
			}
			else
			{
				logInvalidPacket();
			}
			break;
		}
		default:
		{
			logInvalidPacket();
			break;
		}
    }
}

void NetworkManager::setLastShortSleep(const DateTime & shortSleepTime)
{
	m_shortSleepTime = shortSleepTime;
}

void NetworkManager::setLastLongSleep(const DateTime & longSleepTime)
{
	m_longSleepTime = longSleepTime;
}

void NetworkManager::logInvalidPacket()//this is here, in case I want to do something else with invalid packets
{
	Error::log(Error::Number::invalidMessageProtocol);
}
