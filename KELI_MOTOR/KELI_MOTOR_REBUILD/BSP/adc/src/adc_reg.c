#include "adc_reg.h"

volatile float    g_motor_current_mA  = 0.0f;
volatile uint16_t g_motor_current_raw = 0U;

static float s_current_zero_raw = ADC_CURR_VREF_MV * (4095.0f / ADC_VCC_MV);

#define ADC_POT_FILTER_SIZE     16U

/* Maximum allowed deviation between measured zero and theoretical zero.
 * Rejects calibration taken while motor is still coasting (generating back-EMF).
 * 150 raw ~ 24mA: wide enough for real board offsets, tight enough to catch dirty cal. */
#define ADC_CURR_ZERO_MAX_DEV   150.0f

void ADC1_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;   /* read-back to ensure clock is enabled before register access */

    /* PA0, PA1 — analog mode (MODER=11), no pull */
    GPIOA->MODER |= (3U << (0U * 2U)) | (3U << (1U * 2U));

    /* ADC common: ADCPRE=01 → PCLK2/4 = 21MHz */
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |=  (1U << 16U);

    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;
    ADC1->CR2 |= ADC_CR2_ADON;

    ADC1->SQR1 = 0U;   /* L=0: one conversion per trigger */
    ADC1->SQR3 = 0U;

    /* CH0 (potentiometer, RC-filtered, higher source impedance): 144 cycles
     * CH1 (INA240 output, low impedance): 28 cycles */
    ADC1->SMPR2 &= ~((7U << (0U * 3U)) | (7U << (1U * 3U)));
    ADC1->SMPR2 |=  (6U << (0U * 3U))   /* 110 = 144 cycles */
                 |  (2U << (1U * 3U));  /* 010 = 28 cycles */
}

void ADC1_CalibrateCurrentZero(void)
{
    uint32_t zero_sum = 0U;
    uint32_t i;

    (void)ADC1_ReadChannel(1U);   /* discard first sample after channel switch */
    for (i = 0U; i < ADC_CURR_ZERO_SAMPLES; i++) {
        zero_sum += ADC1_ReadChannel(1U);
    }

    float measured   = (float)zero_sum / (float)ADC_CURR_ZERO_SAMPLES;
    float expected   = ADC_CURR_VREF_MV * (4095.0f / ADC_VCC_MV);
    float dev        = (measured > expected) ? (measured - expected) : (expected - measured);

    /* Use measured zero only if deviation is small enough to be credible */
    s_current_zero_raw = (dev <= ADC_CURR_ZERO_MAX_DEV) ? measured : expected;

    g_motor_current_raw = (uint16_t)(s_current_zero_raw + 0.5f);
    g_motor_current_mA  = 0.0f;
}

uint16_t ADC1_ReadChannel(uint8_t ch)
{
    static uint8_t last_ch = 0xFFU;

    ADC1->SQR3 = (uint32_t)(ch & 0x1FU);

    /* Discard one conversion after channel switch to let sampling cap settle.
     * Prevents PA0 potentiometer voltage from contaminating PA1 current sample. */
    if (ch != last_ch) {
        ADC1->CR2 |= ADC_CR2_SWSTART;
        while ((ADC1->SR & ADC_SR_EOC) == 0U) {}
        (void)ADC1->DR;
        last_ch = ch;
    }

    ADC1->CR2 |= ADC_CR2_SWSTART;
    while ((ADC1->SR & ADC_SR_EOC) == 0U) {}
    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

uint16_t ADC1_Read_Filtered(void)
{
    static uint16_t buf[ADC_POT_FILTER_SIZE] = {0U};
    static uint8_t  idx = 0U;
    static uint32_t sum = 0U;

    sum     -= buf[idx];
    buf[idx] = ADC1_ReadChannel(0U);
    sum     += buf[idx];
    idx      = (uint8_t)((idx + 1U) % ADC_POT_FILTER_SIZE);

    return (uint16_t)(sum / ADC_POT_FILTER_SIZE);
}

float ADC1_ReadCurrent_mA(void)
{
    uint32_t raw_sum = 0U;
    uint32_t i;

    /* 192-sample burst (~370us) averages over ~4.5 PWM periods (12.2kHz)
     * to reduce aliasing from async ADC sampling. */
    for (i = 0U; i < ADC_CURR_BURST_SAMPLES; i++) {
        raw_sum += ADC1_ReadChannel(1U);
    }

    float raw_avg   = (float)raw_sum / (float)ADC_CURR_BURST_SAMPLES;
    float raw_delta = raw_avg - s_current_zero_raw;
    float i_ma      = raw_delta * (ADC_VCC_MV / 4095.0f) * 1000.0f
                      / (ADC_CURR_INA240_GAIN * ADC_CURR_SHUNT_MOHM);

    if (i_ma < 0.0f) { i_ma = 0.0f; }   /* unidirectional drive only */

    g_motor_current_raw = (uint16_t)(raw_avg + 0.5f);
    return i_ma;
}

float ADC1_ReadCurrent_Filtered_mA(void)
{
    static float   buf[ADC_CURR_FILTER_SIZE] = {0.0f};
    static uint8_t idx = 0U;
    static float   sum = 0.0f;

    sum     -= buf[idx];
    buf[idx] = ADC1_ReadCurrent_mA();
    sum     += buf[idx];
    idx      = (uint8_t)((idx + 1U) % ADC_CURR_FILTER_SIZE);

    g_motor_current_mA = sum / (float)ADC_CURR_FILTER_SIZE;
    return g_motor_current_mA;
}
