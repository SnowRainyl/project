/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adc_reg.h"
#include "spi_Reg.h"
#include "pid.h"
#include "encoder_reg.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/** 电位器满量程对应的目标转速（RPM），按实际电机最高输出轴转速修改 */
/* 电位器满量程对应的目标转速上限（RPM）。
 * 电机铭牌额定 280 RPM，此处限速 100 RPM（约 36% 额定转速）。 */
#define MOTOR_MAX_RPM           100.0f

/**
 * 速度外环输出的最大电流给定值（mA）
 * 根据 JGA25-370 额定电流和 INA240 + 采样电阻量程设置
 * 超出此值会被速度 PID 积分限幅截断
 */
/* 实测：电机运行时电流 200~300mA，取 400mA 作为内环上限（留裕量） */
#define MOTOR_MAX_CURRENT_MA    400.0f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
static PID_TypeDef speed_pid;    /* 速度外环：setpoint=RPM → output=电流给定(mA) */
static PID_TypeDef current_pid;  /* 电流内环：setpoint=mA  → output=PWM占空比     */
volatile uint16_t  g_pid_duty = 0;   /* 供 main.c 读取，用于串口/OLED 显示 */
volatile uint16_t  g_adc_val  = 0;   /* 电位器原始 ADC 值（0~4095） */

uint16_t sendtofpga;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void Motor_Control_Init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* =============================================================================
 * Motor_Control_Init
 *   初始化 TIM6（1kHz 定时器）和 PID 参数
 *   在 main() 的初始化段调用一次
 *
 * TIM6 时钟链路：
 *   APB1_CLK = 42MHz，APB1 预分频 ≠ 1 → TIM6_CLK = 84MHz
 *   PSC = 83, ARR = 999 → 84MHz / 84 / 1000 = 1kHz (1ms 周期)
 *
 * PID 初始参数（无编码器阶段，kp=1 ki=0 kd=0 = 直通，输出=setpoint）：
 *   接入反馈后再逐步调节 kp/ki/kd。
 * ============================================================================= */
void Motor_Control_Init(void)
{
    /* ---- TIM6 时钟使能（APB1） ---- */
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    /* ---- 计数配置 ---- */
    TIM6->PSC = 83;           /* 84MHz / (83+1) = 1MHz       */
    TIM6->ARR = 999;          /* 1MHz / (999+1) = 1kHz       */
    TIM6->CNT = 0;

    /* ---- 使能更新中断 ---- */
    TIM6->DIER |= TIM_DIER_UIE;

    /* ---- NVIC 配置：优先级 2，允许被 SysTick(15) 打断 ---- */
    NVIC_SetPriority(TIM6_DAC_IRQn, 2);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    /* ---- 启动计数器 ---- */
    TIM6->CR1 |= TIM_CR1_CEN;

    /* ---- 串级 PID 参数初始化 ---- */
    /*
     * 速度外环（Speed Loop）
     *   setpoint : 目标转速（RPM，电位器给定，0~MOTOR_MAX_RPM）
     *   feedback : 编码器实测转速 g_encoder_rpm（RPM）
     *   output   : 期望电流给定（mA），送入电流内环
     *
     *   实测电流范围：75mA（最低速）~ 135mA（最高速）
     *   out_max = MOTOR_MAX_CURRENT_MA = 150mA
     *
     *   kp=1.5 → 100RPM 误差时 P 项 = 150mA（恰好满量程，依赖 anti-windup 限幅）
     *   ki=0.1 → 稳态 100RPM 所需积分 = 100mA/0.1 = 1000 < integral_max=1500 ✓
     *   kd=0.0 → 编码器信号含噪，省略微分
     *   integral_max = out_max / ki = 150 / 0.1 = 1500
     */
    PID_Init(&speed_pid,
             /*kp*/1.5f,  /*ki*/0.1f,  /*kd*/0.0f,
             /*out_min*/0.0f,  /*out_max*/MOTOR_MAX_CURRENT_MA,
             /*integral_max*/4000.0f);  /* integral_max = out_max / ki = 400/0.1 */

    /*
     * 电流内环（Current Loop）
     *   setpoint : 期望电流（mA，速度外环输出，0~150mA）
     *   feedback : INA240 实测电流 g_motor_current_mA（mA）
     *   output   : PWM 占空比（0~4095，送 FPGA）
     *
     *   内环须比外环快 3~5 倍，用较大 kp 加快电流响应。
     *   kp=20  → 150mA 误差时 P 项 = 3000（73% duty），快速响应
     *   ki=0.5 → 稳态 duty≈2482 所需积分 = 2482/0.5 = 4964 < integral_max=8000 ✓
     *   kd=0.0 → 电流信号较干净，省略微分（需要时可加 0.01~0.05）
     *   integral_max = out_max / ki = 4095 / 0.5 = 8190，取 8000
     */
    PID_Init(&current_pid,
             /*kp*/5.0f,  /*ki*/0.2f,  /*kd*/0.0f,  /* kp 从20降到5，避免振荡 */
             /*out_min*/0.0f,  /*out_max*/4095.0f,
             /*integral_max*/8000.0f);

    /* ---- 编码器初始化（TIM3，PC6/PC7） ---- */
    Encoder_Init();
}

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/* =============================================================================
 * TIM6 更新中断 —— 1kHz PID 控制环
 *
 * 执行时间估算（5.25MHz SPI，21MHz ADC）：
 *   ADC 转换 ≈ 4.6μs，SPI 2字节 ≈ 3μs，其余开销 < 1μs → 共约 9μs
 *   占 1ms 周期的 0.9%，完全可接受。
 * ============================================================================= */
void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF) {
        TIM6->SR &= ~TIM_SR_UIF;    /* 清除更新中断标志（必须第一时间清） */

        /* 1. 更新编码器（计算 g_encoder_rpm） */
        Encoder_Update();

        /* 2. 读取电位器，映射为目标转速（0~4095 → 0~MOTOR_MAX_RPM RPM） */
        uint16_t adc_val      = ADC1_Read_Filtered();
        g_adc_val             = adc_val;
        float    setpoint_rpm = (adc_val < 50U) ? 0.0f
                             : (float)adc_val * (MOTOR_MAX_RPM / 4095.0f);

        /* 3. 速度外环 PID → 期望电流（mA） */
        float current_setpoint = PID_Calc(&speed_pid, setpoint_rpm, g_encoder_rpm);

        /* 4. 读取 INA240 实测电流（8 点滤波，同步更新 g_motor_current_mA） */
        float measured_current = ADC1_ReadCurrent_Filtered_mA();

        /* 5. 电流内环 PID → PWM 占空比（0~4095） */
        float    duty_f = PID_Calc(&current_pid, current_setpoint, measured_current);
        uint16_t duty   = (uint16_t)duty_f;
        sendtofpga = duty;
//duty = 1000;
        /* 6. 通过 SPI2 发给 FPGA（2字节，12bit 占空比） */
        uint8_t hi = (duty >> 8U) & 0x0FU;
        uint8_t lo =  duty        & 0xFFU;

        FPGA_CS_LOW();
        SPI2_ReadWriteByte(hi);
        SPI2_ReadWriteByte(lo);
        FPGA_CS_HIGH();

        /* 7. 更新监控变量 */
        g_pid_duty = duty;
    }
}

/* USER CODE END 1 */
