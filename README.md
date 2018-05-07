# ValveControler

This scetch controls when the arduino opens and closes the irrigation valves.
It uses [this](https://github.com/Damjan94/ProgramZaNavodnjavanje) android app for user input
It uses [this](https://github.com/sleemanj/DS3231_Simple) library to tell time.

#hardware

- 1x Arduino nano (ATmega328p)
- 1x HC 06 Bluetooth module
- 1x DS3231 I2C RTC Clock
- 1x Power supply with short circuit protection(for the solenoid)
- 1x Power supply (for arduino)
- 1x 2 channel reley module (H-Bridge)
- 1x 8 channel reley module (for valves, I use 8 valves)
