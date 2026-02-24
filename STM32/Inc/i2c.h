#ifndef I2C_H
#define I2C_H

/*
 * i2c.h — I2C + OLED显示模块
 *
 * 驱动0.96寸 SSD1306 OLED（4针IIC接口，工作电压3.3V）
 * 引脚：PB6(SCL)，PB7(SDA)
 * I2C地址：0x3C（默认）
 *
 * 显示内容：
 *   第1行：目标转速（电位器设定）
 *   第2行：实际转速（编码器测量）
 */

void I2C_Init(void);
void OLED_Init(void);

/* 清屏 */
void OLED_Clear(void);

/* 在指定行显示字符串和数值，例如：显示 "Target: 280 RPM" */
void OLED_ShowSpeed(int target_rpm, int actual_rpm);

#endif /* I2C_H */
