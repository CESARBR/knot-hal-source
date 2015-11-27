/*
 * Copyright (c) 2010 by Cristian Maglie <c.maglie@arduino.cc>
 * Copyright (c) 2014 by Paul Stoffregen <paul@pjrc.com> (Transaction API)
 * Copyright (c) 2014 by Matthijs Kooijman <matthijs@stdin.nl> (SPISettings AVR)
 * Copyright (c) 2014 by Andrew J. Kroll <xxxajk@gmail.com> (atomicity fixes)
 * SPI Master library for arduino.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of either the GNU General Public License version 2
 * or the GNU Lesser General Public License version 2.1, both as
 * published by the Free Software Foundation.
 */
#include "knot_spi.h"

#define ARDUINO
#ifdef ARDUINO

#include <Arduino.h>

static bool m_initialized = false;
static uint8_t m_interruptMode = 0;
static uint8_t m_interruptMask = 0;
static uint8_t m_interruptSave = 0;

uint8_t knot_spi_initialize()
{
	if (initialized) {
		return 1;
	}

	initialized = true;

	uint8_t sreg = SREG;
	noInterrupts(); // Protect from a scheduler and prevent transactionBegin

	// Set it high (to enable the internal pull-up resistor)
	if(!(*portModeRegister(digitalPinToPort(SS)) & digitalPinToBitMask(SS))) {
		digitalWrite(SS, HIGH);
	}

	pinMode(SCK, OUTPUT);
	pinMode(MOSI, OUTPUT);
	pinMode(SS, OUTPUT);

	// Warning: if the SS pin ever becomes a LOW INPUT then SPI
	// automatically switches to Slave, so the data direction of
	// the SS pin MUST be kept as OUTPUT.
	SPCR |= _BV(MSTR);
	SPCR |= _BV(SPE);

	SREG = sreg;

	return 0;
}

void SPIClass::end()
{
	if(initialized)
	{
		initialized = false;

		uint8_t sreg = SREG;
		noInterrupts();

		SPCR &= ~_BV(SPE);
		m_interruptMode = 0;
		SREG = sreg;
}

#ifdef INT0
#define INT0_MASK  (1<<INT0)
#endif
#ifdef INT1
#define INT1_MASK  (1<<INT1)
#endif

void InterruptON(uint8_t intx)
{
	uint8_t mask = 0;
	uint8_t sreg = SREG;

	noInterrupts();
	switch(intx) {
	case INT0:
		mask = 1 << INT0;
		break;
	case INT1:
		mask = 1 << INT1;
		break;
	default:
		interruptMode = 2;
		break;
	}
	m_interruptMask |= mask;
	if(m_interruptMode == 0) {
		m_interruptMode = 1;
	}

	SREG = sreg;
}

void InterruptOFF(uint8_t intx)
{
	if (m_interruptMode != 2) {
		uint8_t mask = 0;
		uint8_t sreg = SREG;

		noInterrupts();
		switch(intx) {
		case INT0:
			mask = 1<<INT0 ;
			break;
		case INT1:
			mask = 1<<INT1;
			break;
		}
		m_interruptMask &= ~mask;
		if(m_interruptMask == 0) {
			m_interruptMode = 0;
		}

		SREG = sreg;
	}
}

// Set exclusive access to the SPI bus and configuring the correct settings.
static void beginTransaction(SPISettings settings)
{
  if (interruptMode > 0) {
    uint8_t sreg = SREG;
    noInterrupts();

    if (interruptMode == 1) {
      interruptSave = EIMSK;
      EIMSK &= ~interruptMask;
      SREG = sreg;
    } else
    {
      interruptSave = sreg;
    }
  }

  #ifdef SPI_TRANSACTION_MISMATCH_LED
  if (inTransactionFlag) {
    pinMode(SPI_TRANSACTION_MISMATCH_LED, OUTPUT);
    digitalWrite(SPI_TRANSACTION_MISMATCH_LED, HIGH);
  }
  inTransactionFlag = 1;
  #endif

  SPCR = settings.spcr;
  SPSR = settings.spsr;
}

#endif
