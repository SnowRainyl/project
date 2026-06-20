#ifndef CUSTOM_I2C_H
#define CUSTOM_I2C_H

#include <stdint.h>

/* Hardware I2C1 — PB6(SCL) / PB7(SDA), AF4, open-drain + pull-up
 * Standard mode 100kHz, APB1=42MHz */

void I2C_Init(void);

/* Write (memadd + data) to slave. Used by OLED driver.
 * slav_add: 7-bit address, memadd: register/control byte, data: single byte */
void i2c_write_memory(uint8_t slav_add, uint8_t memadd, uint8_t data, uint8_t length);

#endif /* CUSTOM_I2C_H */
