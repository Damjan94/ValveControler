#ifndef VALVE_CONTROLLER
#define VALVE_CONTROLLER

#include <Valve.h>
#include <Arduino.h>
#include <DS3231_Simple.h>

class ValveController
{
private:
    
    const static long RELAY_SETTLE_TIME = 10;
    
    int8_t m_hBridgePin[2];
    
    uint8_t m_valveCount;
    Valve m_valves[50];
    uint8_t m_hBridgeState;
    bool m_isHbridgeSet = false;
    
    bool switchHBridge(int8_t state);
    int8_t getHBridgeState();
    
public:
    ValveController();
    bool sendValves(HardwareSerial&);
    bool receiveValves(HardwareSerial&);
    void updateValves(const DateTime& dt);
    int8_t* getHBridgePin();
    DateTime getSoonestActionDate(const DateTime& dt) const;
    void closeAllValves();
};

#endif//include guard
