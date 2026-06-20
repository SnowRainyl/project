#ifndef ADC_REG_H
#define ADC_REG_H

#include "stm32f4xx.h"

/* ADC1 — PA0=IN0 (potentiometer), PA1=IN1 (INA240 current output)
 * Clock: APB2(84MHz)/4 = 21MHz
 * Conversion: software trigger, single, polling EOC */

/* INA240A2: gain=50V/V, shunt=0.1Ohm, REF=VCC/2=1.65V (bidirectional) */
#define ADC_CURR_INA240_GAIN    50.0f
#define ADC_CURR_SHUNT_MOHM     100.0f
#define ADC_CURR_VREF_MV        1650.0f   /* fallback zero if calibration fails */
#define ADC_VCC_MV              3300.0f

#define ADC_CURR_FILTER_SIZE    8U
#define ADC_CURR_ZERO_SAMPLES   64U
#define ADC_CURR_BURST_SAMPLES  192U   /* ~4.5 PWM periods to average out ripple */

extern volatile float    g_motor_current_mA;
extern volatile uint16_t g_motor_current_raw;

void     ADC1_Init(void);

/* Call once after motor is confirmed stopped to calibrate current zero offset */
void     ADC1_CalibrateCurrentZero(void);

uint16_t ADC1_ReadChannel(uint8_t ch);
uint16_t ADC1_Read_Filtered(void);   /* 16-point moving average on potentiometer */
float    ADC1_ReadCurrent_mA(void);
float    ADC1_ReadCurrent_Filtered_mA(void);   /* 8-point MA; updates g_motor_current_mA */

/* Legacy alias */
static inline uint16_t ADC1_Read(void) { return ADC1_ReadChannel(0U); }

#endif /* ADC_REG_H */
