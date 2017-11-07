/*
 * Copyright (c) 2015, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */
#ifdef ARDUINO
#include <stdio.h>
#include <HardwareSerial.h>

static int _putchar(char ch, FILE*)
{
  Serial.write(ch);
  return ch;
}

void printf_serial_init(void)
{
	fdevopen(&_putchar, NULL);
}
#endif // ARDUINO
