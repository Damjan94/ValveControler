#pragma once

#include <stdint.h>
#include <stdio.h>
#include <DS3231_Simple.h>
#include "ValveControler.h"

class NetworkManager
{
public:

	NetworkManager(DS3231_Simple& myClock, ValveController& myController);

    void update(const DateTime& dt);
private:

    DS3231_Simple&		clock;
	ValveController&	valveControler;
	bool				deviceSupportsUs;
};
