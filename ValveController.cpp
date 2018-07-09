#include "ValveController.h"
#include <limits.h>
ValveController::ValveController() : m_hBridgePin{4, 13}
{
    m_isHbridgeSet = true;
    pinMode(m_hBridgePin[0], OUTPUT);
    pinMode(m_hBridgePin[1], OUTPUT);
}


bool ValveController::switchHBridge(int8_t state)
{
    if(!m_isHbridgeSet)
    {
        return false;
    }
    digitalWrite(m_hBridgePin[0], state);
    digitalWrite(m_hBridgePin[1], state);
    m_hBridgeState = state;
    utility::delay(RELAY_SETTLE_TIME);
    return true;
}

int8_t ValveController::getHBridgeState()
{
    return m_hBridgeState;
}
int8_t* ValveController::getHBridgePin()
{
    return m_hBridgePin;
}

bool ValveController::sendValves(HardwareSerial& serial)
{
    serial.write(m_valveCount);
    for(size_t i = 0; i < m_valveCount; ++i)
    {
        m_valves[i].toSerial(serial);
    }
    serial.flush();
    return true;
}

bool ValveController::receiveValves(HardwareSerial& serial)
{
    uint8_t newValveCount = serial.read();
    if(newValveCount > (sizeof(m_valves)/sizeof(Valve)))
    {
        return false;
    }
    //close the valves
    //before assigning the new valve count
    closeAllValves();
    m_valveCount = newValveCount;
    for(size_t i = 0; i < m_valveCount; ++i)
    {
        int tryCount = 0;
        bool isValveInBuffer = true;
        while(serial.available() < Valve::VALVE_NETWORK_SIZE)
        {
                utility::delay(100);
                ++tryCount;
                if(tryCount > 3)
                {
                    while(serial.available() > 0)
                    {
                        serial.read();//purge the stream
                    }
                    isValveInBuffer = false;
                }
        }
        if(!isValveInBuffer)
        break;
        m_valves[i].fromSerial(serial);
        //Serial.write(Message::VALVE_RECEIVED);
    }
    return true;
}

void ValveController::updateValves(const DateTime& dt)
{
    bool hBridgeSetToTurnOn = false;
    for(size_t i = 0; i < m_valveCount; ++i)
    {
        if(m_valves[i].checkTurnOn(dt))
        {
            if(!hBridgeSetToTurnOn)
            {
                switchHBridge(LOW);
                hBridgeSetToTurnOn = true;
            }
            m_valves[i].turnOn(dt);
        }
    }
    bool hBridgeSetToTurnoff = false;
    for(size_t i = 0; i < m_valveCount; ++i)
    {
        if(m_valves[i].checkTurnOff(dt))
        {
            if(!hBridgeSetToTurnoff)
            {
                switchHBridge(HIGH);
                hBridgeSetToTurnoff = true;
            }
            m_valves[i].turnOff();
        }
    }

    if(getHBridgeState() != HIGH)
    {
        switchHBridge(HIGH);
    }
}

DateTime ValveController::getSoonestActionDate(const DateTime& dt) const
{
    int soonestAction = INT_MAX;
    for(int i = 0; i < m_valveCount; ++i)
    {
        int valveAction = m_valves[i].getActionTime(dt);
        if(valveAction < soonestAction)
        {
            soonestAction = valveAction;
        }
    }
    return utility::addMinutesToDate(soonestAction, dt);
}

void ValveController::closeAllValves()
{
    //TODO fix this code. 5 and 12 are the valid valve pins
    for(int i = 5; i <= 12; ++i)
    {
        switchHBridge(HIGH);
        digitalWrite(i, LOW);
        utility::delay(15);
        digitalWrite(i, HIGH);
    }
}
