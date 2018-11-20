#ifndef VALVE_CONTROLLER
#define VALVE_CONTROLLER

#include <Valve.h>
#include <Arduino.h>
#include <DS3231_Simple.h>

class ValveController
{
protected:
    
    enum HBridgeState:uint8_t
    {
        close   = HIGH, //set this hbridge to close the valve
        open    = LOW   //---------||--------- open the valve
    };

    const static long RELAY_SETTLE_TIME = 10;
    
    int8_t m_hBridgePin[2];
    
    uint8_t m_valveCount;
    Valve m_valves[50];
    uint8_t m_hBridgeState;
    
    bool m_isHbridgeSet;
    bool m_safeToChangeValves;//set to true after close all valves is called

    bool setHBridge(HBridgeState state);
    int8_t getHBridgeState();
    
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

#endif//include guard
