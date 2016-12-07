/*
 * Copyright (c) 2016, CESAR.
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD license. See the LICENSE file for details.
 *
 */

#ifndef __HAL_GPIO_H__
#define __HAL_GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

void hal_gpio_pin_mode(uint8_t, uint8_t);
void hal_gpio_digital_write(uint8_t, uint8_t);
int hal_gpio_digital_read(uint8_t);
int hal_gpio_analog_read(uint8_t);
void hal_gpio_analog_reference(uint8_t mode);
void hal_gpio_analog_write(uint8_t, int);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_GPIO_H__ */
