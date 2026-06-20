#include "encoder_reg.h"

volatile int16_t g_encoder_delta = 0;
volatile float   g_encoder_rpm   = 0.0f;

static uint16_t s_last_cnt = ENCODER_CNT_CENTER;
static float    s_rpm_buf[ENCODER_FILTER_SIZE];
static uint8_t  s_rpm_idx = 0U;
static float    s_rpm_sum = 0.0f;

static float RPM_Filter(float raw)
{
    s_rpm_sum -= s_rpm_buf[s_rpm_idx];
    s_rpm_buf[s_rpm_idx] = raw;
    s_rpm_sum += raw;
    s_rpm_idx  = (uint8_t)((s_rpm_idx + 1U) % ENCODER_FILTER_SIZE);
    return s_rpm_sum / (float)ENCODER_FILTER_SIZE;
}

void Encoder_Init(void)
{
    uint32_t i;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->APB1ENR;

    /* PC6, PC7 — AF2 (TIM3), pull-up, high speed */
    GPIOC->MODER   &= ~((3UL << (6U * 2U)) | (3UL << (7U * 2U)));
    GPIOC->MODER   |=  ((2UL << (6U * 2U)) | (2UL << (7U * 2U)));
    GPIOC->OSPEEDR |=  ((3UL << (6U * 2U)) | (3UL << (7U * 2U)));
    GPIOC->PUPDR   &= ~((3UL << (6U * 2U)) | (3UL << (7U * 2U)));
    GPIOC->PUPDR   |=  ((1UL << (6U * 2U)) | (1UL << (7U * 2U)));
    GPIOC->AFR[0]  &= ~((0xFUL << (6U * 4U)) | (0xFUL << (7U * 4U)));
    GPIOC->AFR[0]  |=  ((0x2UL << (6U * 4U)) | (0x2UL << (7U * 4U)));

    TIM3->PSC = 0U;
    TIM3->ARR = ENCODER_TIM_ARR;

    /* Encoder mode 3: both TI1 and TI2 edges count (x4 decode)
     * IC1F/IC2F=0001: 2-sample digital filter to reject glitches */
    TIM3->CCMR1 = (1UL << 0U)    /* CC1S=01: IC1 mapped to TI1 */
                | (1UL << 4U)    /* IC1F=0001 */
                | (1UL << 8U)    /* CC2S=01: IC2 mapped to TI2 */
                | (1UL << 12U);  /* IC2F=0001 */
    TIM3->CCER  = 0U;
    TIM3->SMCR  = (3UL << 0U);   /* SMS=011: encoder mode 3 */

    TIM3->CNT = ENCODER_CNT_CENTER;
    s_last_cnt = ENCODER_CNT_CENTER;

    s_rpm_idx = 0U;
    s_rpm_sum = 0.0f;
    for (i = 0U; i < ENCODER_FILTER_SIZE; i++) { s_rpm_buf[i] = 0.0f; }

    TIM3->CR1 |= TIM_CR1_CEN;
}

/* Call from TIM6 ISR every 10ms (ENCODER_UPDATE_HZ = 100Hz) */
void Encoder_Update(void)
{
    uint16_t cnt = (uint16_t)TIM3->CNT;

    /* Signed 16-bit subtraction handles 16-bit counter wrap-around automatically */
    g_encoder_delta = (int16_t)(cnt - s_last_cnt);
    s_last_cnt = cnt;

    float raw_rpm = (float)g_encoder_delta
                  * (60.0f * (float)ENCODER_UPDATE_HZ)
                  / (float)ENCODER_COUNTS_PER_REV;

    g_encoder_rpm = RPM_Filter(raw_rpm);
}

int32_t Encoder_GetCount(void)
{
    return (int32_t)(int16_t)((uint16_t)TIM3->CNT - ENCODER_CNT_CENTER);
}

void Encoder_ResetCount(void)
{
    uint32_t i;

    TIM3->CNT  = ENCODER_CNT_CENTER;
    s_last_cnt = ENCODER_CNT_CENTER;
    g_encoder_delta = 0;
    g_encoder_rpm   = 0.0f;
    s_rpm_idx = 0U;
    s_rpm_sum = 0.0f;
    for (i = 0U; i < ENCODER_FILTER_SIZE; i++) { s_rpm_buf[i] = 0.0f; }
}

float Encoder_GetSpeed_RPM(void)
{
    return g_encoder_rpm;
}
