#include "adc_reg.h"

/* =============================================================================
 * ADC1 寄存器级驱动
 *
 * 硬件资源：
 *   PA0 → ADC1_IN0（电位器，目标转速给定）
 *   PA1 → ADC1_IN1（INA240 输出，电机实际电流反馈）
 *
 * 时钟链路：
 *   HSI(16MHz) → PLL → SYSCLK=168MHz → APB2=84MHz → ADCPRE=/4 → ADC=21MHz
 *
 * 转换策略：
 *   软件触发，单次转换，每次调用前动态修改 SQR3 切换通道，轮询 EOC。
 *   两次转换合计约 8μs，占 1ms 控制周期的 0.8%，完全可接受。
 *
 * INA240 电流换算（在 adc_reg.h 中配置硬件参数）：
 *   V_out  = raw_adc × VCC / 4095           [mV]
 *   I      = (V_out − V_ref) / (Gain × R_shunt)
 *   I_mA   = (V_out_mV − VREF_mV) × 1000 / (GAIN × SHUNT_mΩ)
 * ============================================================================= */

/* ---- 全局变量定义 ---- */
volatile float g_motor_current_mA = 0.0f;
volatile uint16_t g_motor_current_raw = 0U;
static float s_current_zero_raw =
    ADC_CURR_VREF_MV * (4095.0f / ADC_VCC_MV);

/* ---- 电位器滤波器内部使用 ---- */
#define ADC_POT_FILTER_SIZE  16U

/* =============================================================================
 * ADC1_Init
 * ============================================================================= */
void ADC1_Init(void)
{
    /* ------------------------------------------------------------------
     * 1. 开启时钟
     * ------------------------------------------------------------------ */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;   /* GPIOA */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;    /* ADC1  */
    (void)RCC->APB2ENR;                     /* 读回确保时序 */

    /* ------------------------------------------------------------------
     * 2. GPIO 模拟输入（MODER = 11，模拟模式，无需上下拉）
     *    PA0 → ADC1_IN0（电位器）
     *    PA1 → ADC1_IN1（INA240 输出）
     * ------------------------------------------------------------------ */
    GPIOA->MODER |= (3U << (0U * 2U))   /* PA0: 11 */
                 |  (3U << (1U * 2U));  /* PA1: 11 */

    /* ------------------------------------------------------------------
     * 3. ADC 公共寄存器 CCR：时钟 = PCLK2 / 4 = 21MHz
     *    ADCPRE[1:0] = 01
     * ------------------------------------------------------------------ */
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |=  (1U << 16U);           /* 01: /4 */

    /* ------------------------------------------------------------------
     * 4. CR1：12 位分辨率，关闭扫描
     * ------------------------------------------------------------------ */
    ADC1->CR1 = 0U;

    /* ------------------------------------------------------------------
     * 5. CR2：单次转换，右对齐，软件触发，上电
     * ------------------------------------------------------------------ */
    ADC1->CR2 = 0U;
    ADC1->CR2 |= ADC_CR2_ADON;

    /* ------------------------------------------------------------------
     * 6. 序列寄存器：每次仅转换 1 个通道（L=0，SQ1 动态切换）
     * ------------------------------------------------------------------ */
    ADC1->SQR1 = 0U;   /* L=0 → 1次转换 */
    ADC1->SQR3 = 0U;   /* 默认 SQ1=CH0  */

    /* ------------------------------------------------------------------
     * 7. 采样时间
     *    CH0（电位器，阻抗 < 10kΩ）：84 周期（SMP0=110，约 4μs）
     *    CH1（INA240 输出，低阻抗）  ：28 周期（SMP1=010，约 1.3μs）
     *
     *    SMPR2 布局：SMP_n 在 bits[(n*3+2):(n*3)]，n=0~9
     * ------------------------------------------------------------------ */
    ADC1->SMPR2 &= ~((7U << (0U * 3U)) | (7U << (1U * 3U)));
    ADC1->SMPR2 |=  (6U << (0U * 3U))   /* CH0: 110 = 84 cycles */
                 |  (2U << (1U * 3U));  /* CH1: 010 = 28 cycles */

}

/*
 * 标定值与理论零点（REF 对应 raw）的最大允许偏差。
 * 超出即视为标定环境不干净（典型：复位时电机仍在惯性续流发电，把发电电流
 * 当成零点存入），拒绝该值、回退到理论零点，避免幽灵电流锁死电流内环。
 * 150 raw ≈ 24mA，足以容忍板间真实零偏，又能拦截 >100mA 级别的脏标定。
 */
#define ADC_CURR_ZERO_MAX_DEV    150.0f

void ADC1_CalibrateCurrentZero(void)
{
    uint32_t zero_sum = 0U;
    uint32_t i;

    (void)ADC1_ReadChannel(1U);
    for (i = 0U; i < ADC_CURR_ZERO_SAMPLES; i++) {
        zero_sum += ADC1_ReadChannel(1U);
    }

    float measured_zero   = (float)zero_sum / (float)ADC_CURR_ZERO_SAMPLES;
    float expected_zero   = ADC_CURR_VREF_MV * (4095.0f / ADC_VCC_MV);
    float dev             = measured_zero - expected_zero;
    if (dev < 0.0f) {
        dev = -dev;
    }

    /* 偏差过大 → 标定环境不可信（电机未停稳），回退理论零点。 */
    s_current_zero_raw = (dev <= ADC_CURR_ZERO_MAX_DEV)
                       ? measured_zero
                       : expected_zero;

    g_motor_current_raw = (uint16_t)(s_current_zero_raw + 0.5f);
    g_motor_current_mA = 0.0f;
}

/* =============================================================================
 * ADC1_ReadChannel：切换到指定通道，触发单次转换，返回 12 位原始值
 *
 * @param ch  通道号，0 = IN0（电位器），1 = IN1（电流）
 * ============================================================================= */
uint16_t ADC1_ReadChannel(uint8_t ch)
{
    static uint8_t last_ch = 0xFFU;

    /* 动态设置本次转换的通道 */
    ADC1->SQR3 = (uint32_t)(ch & 0x1FU);

    /*
     * 通道切换后先做一次丢弃转换，让内部采样电容稳定。
     * 可避免 PA0 电位器电压串入紧随其后的 PA1 电流采样。
     */
    if (ch != last_ch) {
        ADC1->CR2 |= ADC_CR2_SWSTART;
        while ((ADC1->SR & ADC_SR_EOC) == 0U) {
        }
        (void)ADC1->DR;
        last_ch = ch;
    }

    /* 软件触发 → 等待 EOC → 读取有效结果 */
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while ((ADC1->SR & ADC_SR_EOC) == 0U) {
    }
    return (uint16_t)(ADC1->DR & 0x0FFFU);
}

/* =============================================================================
 * ADC1_Read_Filtered：电位器通道 16 点滑动均值滤波
 *
 * 滤波延迟 = 16 次调用周期（16ms @ 1kHz）
 * ============================================================================= */
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

/* =============================================================================
 * ADC1_ReadCurrent_mA：电流通道单次采样 → 换算为毫安
 *
 * 换算公式：
 *   V_out_mV = raw × ADC_VCC_MV / 4095
 *   I_mA     = (V_out_mV − ADC_CURR_VREF_MV) × 1000
 *              / (ADC_CURR_INA240_GAIN × ADC_CURR_SHUNT_MOHM)
 *
 * 结果可为负（双向电流测量时表示反向电流）。
 * ============================================================================= */
float ADC1_ReadCurrent_mA(void)
{
    uint32_t raw_sum = 0U;
    uint32_t i;

    /*
     * SPI/FPGA 产生约 12.2kHz PWM，一个周期约 82us。
     * 单点 ADC 会随机采到导通、续流或关断阶段，造成严重混叠。
     * 连续采 192 点约 370us，覆盖约 4.5 个 PWM 周期，降低异步采样混叠。
     */
    for (i = 0U; i < ADC_CURR_BURST_SAMPLES; i++) {
        raw_sum += ADC1_ReadChannel(1U);
    }

    float raw_avg = (float)raw_sum / (float)ADC_CURR_BURST_SAMPLES;
    float raw_delta = raw_avg - s_current_zero_raw;
    float i_ma = raw_delta * (ADC_VCC_MV / 4095.0f) * 1000.0f
                 / (ADC_CURR_INA240_GAIN * ADC_CURR_SHUNT_MOHM);

    /* 当前控制只允许正向驱动，负值来自零点误差或 PWM 续流采样。 */
    if (i_ma < 0.0f) {
        i_ma = 0.0f;
    }

    g_motor_current_raw = (uint16_t)(raw_avg + 0.5f);
    return i_ma;
}

/* =============================================================================
 * ADC1_ReadCurrent_Filtered_mA：8 点移动平均，同步更新 g_motor_current_mA
 * ============================================================================= */
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
