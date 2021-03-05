# ATtiny I²C Fan Control

Control a PWM fan over the I²C bus.

**Tests:** ![Test and Release](https://github.com/crycode-de/attiny85-fan-control/workflows/PlatformIO%20CI/badge.svg)

*ATtiny I²C Fan Control* is made as a [PlatformIO](https://platformio.org/) project.

## Supported features

* Set fan speed by using a PWM signal
* Set fan speed via I²C bus
* Read the current fan RPM via I²C bus
* Auto adjust PWM range by testing the connected fan at startup (can be re-triggered by I²C bus)
* Override min PWM level by I²C bus

## Currently supported AVR controllers

* [ATtiny85](https://ww1.microchip.com/downloads/en/DeviceDoc/Atmel-2586-AVR-8-bit-Microcontroller-ATtiny25-ATtiny45-ATtiny85_Datasheet.pdf)

Adjustments for other AVR microcontrollers should be easy.

## Wiring

![Wiring](./doc/attiny-i2c-fan-control-wiring.png)

## Projects using ATtiny I²C Fan Control

* [Active cooling of the "HomePi"](https://crycode.de/homepi-kuehlung) (German)

## Used Libraries

* [Arduino Framework](https://platformio.org/frameworks/arduino)
* [PinChangeInterrupt](https://platformio.org/lib/show/725/PinChangeInterrupt)
* [TinyWireSio](https://platformio.org/lib/show/293/TinyWireSio)

## License

**CC BY-NC-SA 4.0**

[Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-nc-sa/4.0/)

Copyright (C) 2020-2021 Peter Müller <peter@crycode.de> [https://crycode.de](https://crycode.de)