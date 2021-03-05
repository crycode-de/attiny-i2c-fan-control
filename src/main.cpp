/**
 * ATtiny I²C Fan Control
 *
 * Control a PWM fan over the I²C bus.
 *
 * Copyright (C) 2020-2021 Peter Müller <peter@crycode.de> (https://crycode.de)
 * License: CC BY-NC-SA 4.0
 */

#include "config.h"
#include <PinChangeInterrupt.h>
#include <TinyWireS.h>

/**
 * Buffer size of the I2C RX buffer.
 */
#define TWI_RX_BUFFER_SIZE ( 16 )

/**
 * Multiplicator for rounds per second (RPS) counting.
 * This is the number of seconds to count the tacho impulses before re-calculate
 * the RPS/RPM of the fan.
 */
#define RPS_MULTI 3

/**
 * Macro to check the time for time-based events.
 * If a is greater than or equal to b this returns true, otherwise false.
 * This resprects a possible rollover of a and b.
 */
#define checkTime(a, b) ((long)(a - b) >= 0)

bool ledStatus = false;

volatile uint16_t rps = 0;
uint16_t rpm = 0;

unsigned long now = 0;
unsigned long timeNextRpmCalc = 0;
unsigned long timepwmLevelMinCalNextStep = 0;

uint8_t pwmLevel = 0;
uint8_t pwmLevelMin = 0;
bool pwmLevelMinCal = false;
bool pwmLevelMinCalStart = false;

bool blinkLed = false;

// The "registers" we expose to I2C
volatile uint8_t i2c_regs[] = {
  0x00, // 0x00 - Status register
  0x00, // 0x01 - set speed command
  0x00, // 0x02 - min pwm
  0x00, // 0x03 - tacho rps
  0x00, // 0x04 - tacho low byte
  0x00, // 0x05 - tacho high byte
};

const uint8_t reg_size = sizeof(i2c_regs);
// Tracks the current register pointer position
volatile uint8_t reg_position;

/**
 * Set the fan speed.
 * @param speed The speed from 0 (off) to 255 (full speed).
 */
void setFanSpeed (uint8_t speed) {
  if (speed > 0) {
    pwmLevel = map(speed, 0, 255, pwmLevelMin, 255);
  } else {
    pwmLevel = 0;
  }
  analogWrite(PIN_PWM, pwmLevel);
}

/**
 * This is called for each read request we receive, never put more than one byte of data (with TinyWireS.send) to the
 * send-buffer when using this callback
 */
void i2cRequestEvent() {
    TinyWireS.send(i2c_regs[reg_position]);
    // Increment the reg position on each read, and loop back to zero
    reg_position++;
    if (reg_position >= reg_size)
    {
        reg_position = 0;
    }
}

/**
 * The I2C data received-handler
 *
 * This needs to complete before the next incoming transaction (start, data,
 * restart/stop) on the bus occurs.
 *
 * To be quick, set flags for long running tasks to be called from the mainloop
 * instead of running them directly.
 */
void i2cReceiveEvent (uint8_t howMany) {
  if (howMany < 1 || howMany > TWI_RX_BUFFER_SIZE) {
    // Sanity-check
    return;
  }

  blinkLed = true;

  reg_position = TinyWireS.receive();
  howMany--;
  if (!howMany) {
      // This write was only to set the buffer for next read
      return;
  }

  while (howMany--) {
    if (reg_position == 0x00 && !pwmLevelMinCal && !pwmLevelMinCalStart) {
      // status register
      i2c_regs[0x00] = TinyWireS.receive() & 0b00000001; // only interested in bit 0
      if (bitRead(i2c_regs[0x00], 0)) {
        // if bit 0 is set start calibration
        pwmLevelMinCalStart = true;
      }
    } else if (reg_position == 0x01 && !pwmLevelMinCal && !pwmLevelMinCalStart) {
      // fan speed register
      i2c_regs[0x01] = TinyWireS.receive();
      setFanSpeed(i2c_regs[0x01]);
    } else if (reg_position == 0x02 && !pwmLevelMinCal && !pwmLevelMinCalStart) {
      // pwm min register
      i2c_regs[0x02] = TinyWireS.receive();
      pwmLevelMin = i2c_regs[0x02];
      setFanSpeed(i2c_regs[0x01]);
    } else {
      // no writeable register... just read and discard data
      TinyWireS.receive();
    }

    reg_position++;
    if (reg_position >= reg_size) {
      reg_position = 0;
    }
  }
}

/**
 * ISR to handle tacho impulses.
 */
void handlePcintTacho (void) {
  rps++;
}

/**
 * Setup function.
 */
void setup() {
  pinMode(LED, OUTPUT);
  pinMode(PIN_PWM, OUTPUT);

  attachPCINT(digitalPinToPCINT(PIN_TACHO), handlePcintTacho, FALLING);

  TinyWireS.begin(I2C_SLAVE_ADDRESS);
  TinyWireS.onReceive(i2cReceiveEvent);
  TinyWireS.onRequest(i2cRequestEvent);

  now = millis();
  timeNextRpmCalc = now + 1000 * RPS_MULTI;

  setFanSpeed(0);
  pwmLevelMinCalStart = true;
}

/**
 * Main loop.
 */
void loop() {
  TinyWireS_stop_check();

  // need to blink the LED?
  if (blinkLed) {
    digitalWrite(LED, HIGH);
    tws_delay(50);
    digitalWrite(LED, LOW);
    blinkLed = false;
  }

  now = millis();

  if (checkTime(now, timeNextRpmCalc)) {
    rpm = rps * 60 / RPS_MULTI;
    i2c_regs[3] = (rps / RPS_MULTI) & 0xff;
    i2c_regs[4] = rpm & 0xff;
    i2c_regs[5] = (rpm >> 8) & 0xff;

    rps = 0;
    timeNextRpmCalc = now + 1000 * RPS_MULTI;

    if (pwmLevelMinCalStart) {
      // should start calibration... need to stop the fan first in 2 loop runs... third run will start calibration
      if (!pwmLevelMinCal) {
        // first loop run stop fan and set flag to run calibration
        setFanSpeed(0);
        bitSet(i2c_regs[0x00], 0);
        digitalWrite(LED, HIGH);
        pwmLevelMinCal = true;
      } else {
        // second loop run unset flag for calibration start
        pwmLevelMinCalStart = false;
      }

    } else if (pwmLevelMinCal) {
      if (rpm > 200) {
        // fan is rotating with at least 200rpm ... calibration done! :-)
        pwmLevelMin = pwmLevel + 5; // add 5 to be sure that the fan will rotate
        pwmLevelMinCal = false;
        i2c_regs[0x02] = pwmLevelMin;
        bitClear(i2c_regs[0x00], 0);
        setFanSpeed(0);
        digitalWrite(LED, LOW);

      } else {
        // fan not rotating
        // increase pwm
        pwmLevel += 5;
        analogWrite(PIN_PWM, pwmLevel);
        bitSet(i2c_regs[0x00], 0);
        digitalWrite(LED, HIGH);
      }

    }
  }

}
