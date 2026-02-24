#ifndef ADC_H
#define ADC_H

/*
 * adc.h — ADC采样模块
 *
 * 通道1（PA1）：读取电位器电压 → 换算为目标转速
 * 通道2（PA2）：读取INA240输出电压 → 换算为实际电流
 *
 * ADC精度：12位，参考电压3.3V
 * 对电位器输出加了RC硬件低通滤波 + 软件滑动平均滤波
 */

void ADC_Init(void);

/* 读取电位器，返回目标转速（RPM） */
int ADC_ReadTargetSpeed(void);

/* 读取INA240输出，返回实际电流（mA） */
int ADC_ReadMotorCurrent(void);

#endif /* ADC_H */
