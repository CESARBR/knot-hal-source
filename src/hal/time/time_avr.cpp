/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#include <Arduino.h>
#include <limits.h>

#include "time.h"

uint32_t hal_time_ms(void)
{
	return millis();
}

uint32_t hal_time_us(void)
{
	return micros();
}

void hal_delay_ms(uint32_t ms)
{
	delay(ms);
}

void hal_delay_us(uint32_t us)
{
	delayMicroseconds(us);
}

int hal_timeout(uint32_t current,  uint32_t start,  uint32_t timeout)
{
	/* Time overflow */
	if (current < start)
		/* Fit time overflow and compute time elapsed */
		current += (ULONG_MAX - start);
	else
		/* Compute time elapsed */
		current -= start;

	/* Timeout is flagged */
	return (current >= timeout);
}
