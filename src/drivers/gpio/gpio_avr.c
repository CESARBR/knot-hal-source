/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <Arduino.h>

#include "include/gpio.h"

void hal_gpio_pin_mode(uint8_t pin, uint8_t mode)
{
	return pinMode(pin, mode);
}

void hal_gpio_digital_write(uint8_t pin, uint8_t value)
{
	digitalWrite(pin, value);
}

int hal_gpio_digital_read(uint8_t pin)
{
	return digitalRead(pin);
}

int hal_gpio_analog_read(uint8_t pin)
{
	return analogRead(pin);
}

void hal_gpio_analog_reference(uint8_t type)
{
	analogReference(type);
}

void hal_gpio_analog_write(uint8_t pin, int value)
{
	analogWrite(pin, value);
}

