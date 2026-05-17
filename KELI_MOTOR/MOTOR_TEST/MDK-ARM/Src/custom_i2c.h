#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/*
 * i2c.h — 寄存器级 I2C1 驱动（硬件I2C，仅写操作）
 *
 * 硬件：PB6(SCL), PB7(SDA)，AF4，开漏+上拉
 * I2C地址：0x3C（SSD1306 OLED默认）
 */

void I2C_Init(void);

/* 写内存（SSD1306驱动使用）：slav_add 7位地址，memadd 寄存器地址，data 数据，length 数据长度 */
void i2c_write_memory(uint8_t slav_add, uint8_t memadd, uint8_t data, uint8_t len);

#endif /* I2C_H */
