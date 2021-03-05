/**
 * ATtiny I²C Fan Control
 *
 * Control a PWM fan over the I²C bus.
 *
 * Copyright (C) 2020-2021 Peter Müller <peter@crycode.de> (https://crycode.de)
 * License: CC BY-NC-SA 4.0
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <Arduino.h>

/**
 * I²C slave address of the controller.
 */
#define I2C_SLAVE_ADDRESS 0x66

/**
 *  Pin for the PWM output.
 */
#define PIN_PWM 4 // PB4

/**
 * Pin for tacho input.
 */
#define PIN_TACHO 1 // PB1

/**
 * Pin for LED output.
 */
#define LED 3 // PB3

#endif
