#pragma once

#include "Valve.h"
#include <Arduino.h>
#include <DS3231_Simple.h>

class ValveController
{
protected:
    
    enum HBridgeState:uint8_t
    {
        close   = HIGH, //set this hbridge to close the valve
        open    = LOW   //set this hbridge to open the valve
    };

public:
    const static long RELAY_SETTLE_TIME = 10;
	const static int MAX_VALVES = 50;
	const static uint8_t AVAILABLE_PINS = 8;
protected:

    int8_t m_hBridgePin[2];
    
    uint8_t m_valveCount;
    Valve m_valves[MAX_VALVES];
	HBridgeState m_hBridgeState;
    
    bool m_isHbridgeSet;
    bool m_safeToChangeValves;//set to true after close all valves is called
	bool m_usedPins[AVAILABLE_PINS];

    bool setHBridge(HBridgeState state);
	HBridgeState getHBridgeState();
    
public:
    ValveController();
    void update(const DateTime& dt);
    int8_t* getHBridgePin();
    
    //const Valve* getValves() const;
    //const Valve& getValve(size_t index) const;
    void setValve(size_t index, const Valve& valve);
    void addValve(const Valve& valve);
   
    const Valve* getValves() const;
    const Valve* getValve(size_t index) const;
    size_t getValveCount() const;

    void clear();
    
    DateTime getSoonestActionDate(const DateTime& dt) const;
    void closeAllValves();
};
