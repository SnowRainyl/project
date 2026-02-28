#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/*
 * i2c.h — I2C + OLED显示模块
 *
 * 驱动0.96寸 SSD1306 OLED（4针IIC接口，工作电压3.3V）
 * 引脚：PB6(SCL)，PB7(SDA)
 * I2C地址：0x3C（默认）
 *
 * 此文件还包含一个简单的寄存器级 I2C 驱动，可直接使用
 * PB6/PB7 控制硬件 I2C1，供其它外设或手工通信时使用。
 *
 * 显示内容：
 *   简单示例字符串，由调用者通过 OLED_ShowString 传入。
 */

void I2C_Init(void);

/* 原始寄存器级写内存函数，SSD1306驱动使用 */
void i2c_write_memory(uint8_t slav_add, uint8_t memadd, uint8_t data, uint8_t len);

/* OLED相关接口 */


/* 清屏 */


/* 简单示范：清屏并显示传入的 ASCII 字符串 */
/* 低级 I2C 寄存器接口 */
void I2C_Init(void);
void I2C_Start(void);
void I2C_Stop(void);
void I2C_SendByte(unsigned char byte);



#endif /* I2C_H */
