#include "ValveControler.h"
#include <limits.h>
	
ValveController::ValveController() : m_hBridgePin{4, 13}
{
    m_isHbridgeSet = true;
    pinMode(m_hBridgePin[0], OUTPUT);
    pinMode(m_hBridgePin[1], OUTPUT);
}


bool ValveController::setHBridge(HBridgeState state)
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

void ValveController::update(const DateTime& dt)
{
    bool hBridgeSetToTurnOn = false;
    for(size_t i = 0; i < m_valveCount; ++i)
    {
        if(m_valves[i].checkTurnOn(dt))
        {
            if(!hBridgeSetToTurnOn)
            {
                setHBridge(HBridgeState::open);
                hBridgeSetToTurnOn = true;
            }
            m_valves[i].turnOn(dt);
            m_safeToChangeValves = false;
        }
    }
    bool hBridgeSetToTurnoff = false;
    for(size_t i = 0; i < m_valveCount; ++i)
    {
        if(m_valves[i].checkTurnOff(dt))
        {
            if(!hBridgeSetToTurnoff)
            {
                setHBridge(HBridgeState::close);
                hBridgeSetToTurnoff = true;
            }
            m_valves[i].turnOff();
        }
    }

    if(getHBridgeState() != HIGH)
    {
        setHBridge(HBridgeState::close);
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
        setHBridge(HBridgeState::close);
        digitalWrite(i, LOW);
        utility::delay(15);
        digitalWrite(i, HIGH);
    }
    m_safeToChangeValves = true;
}

size_t ValveController::getValveCount() const
{
    return m_valveCount;
}

const Valve* ValveController::getValves() const
{
    return m_valves;
}

const Valve* ValveController::getValve(size_t index) const
{
    if(m_valveCount < index)
        return nullptr;
    return &m_valves[index];
}   

void ValveController::setValve(size_t index, const Valve& valve)
{
    if(!m_safeToChangeValves)
        return;
    if(m_valveCount < index)
        return;
    m_valves[index] = valve;
}

void ValveController::addValve(const Valve& valve)
{
    if(!m_safeToChangeValves)
        return;
    if(m_valveCount+1 >= 50)
        return; 
    m_valves[m_valveCount++] = valve;
}

void ValveController::clear()
{
    closeAllValves();
    m_valveCount = 0;
}
