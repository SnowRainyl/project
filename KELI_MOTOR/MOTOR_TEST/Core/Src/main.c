/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "uart.h"
#include "pid.h"
#include "stm32f4xx_it.h"
#include "custom_i2c.h"
#include "OLED_SSD1306.h"
#include "spi_Reg.h"
#include "w25q64.h"
#include "adc_reg.h"

/* 来自 stm32f4xx_it.c 的外部符号 */
extern void              Motor_Control_Init(void);
extern volatile uint16_t g_pid_duty;
extern volatile uint16_t g_adc_val;
extern volatile float    g_encoder_rpm;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static char uart_buf[128];

/* Flash 连接状态（1 = ID 正确 0xEF16） */
uint16_t flash_id    = 0;
uint8_t  flash_id_ok = 0;

/* ===== PID 参数 Flash 存储 ===== */
#define PID_FLASH_ADDR    0x000000UL   /* 回到扇区0（唯一确认过写入成功的地址） */
#define PID_FLASH_MAGIC   12345.678f   /* 魔数：Flash 擦除后全0xFF，读回!=此值说明未写过 */
static float pid_flash_buf[8];         /* [magic, sp_kp, sp_ki, sp_kd, cp_kp, cp_ki, cp_kd, 0] */

/* ===== 串口命令行缓冲 ===== */
static char    cmd_buf[64];
static uint8_t cmd_len = 0;

static void PID_SaveToFlash(void) {
    if (!flash_id_ok) {
        UART_SendString("[PID] Flash未连接，save跳过\r\n");
        return;
    }
    pid_flash_buf[0] = PID_FLASH_MAGIC;
    pid_flash_buf[1] = speed_pid.kp;
    pid_flash_buf[2] = speed_pid.ki;
    pid_flash_buf[3] = speed_pid.kd;
    pid_flash_buf[4] = current_pid.kp;
    pid_flash_buf[5] = current_pid.ki;
    pid_flash_buf[6] = current_pid.kd;
    pid_flash_buf[7] = 0.0f;
    /* 擦除前：读 SR1+SR2，重点检查 CMP(SR2 bit6) 是否为1
     * CMP=1 且 BP=000 → 全片保护，所有写/擦除静默失败，WEL保持1 */
    uint8_t sr1_before = W25Q64_ReadSR1();
    uint8_t sr2_before = W25Q64_ReadSR2();
    snprintf(uart_buf, sizeof(uart_buf),
             "[PID] SR1=0x%02X SR2=0x%02X (WEL=%d BP=%d CMP=%d SRP1=%d)\r\n",
             sr1_before, sr2_before,
             (sr1_before >> 1) & 1,
             (sr1_before >> 2) & 7,
             (sr2_before >> 6) & 1,   /* CMP */
             sr2_before & 1);         /* SRP1 */
    UART_SendString(uart_buf);

    W25Q64_Erase_Sector(PID_FLASH_ADDR);
    W25Q64_Write_4Floats(PID_FLASH_ADDR,      &pid_flash_buf[0]);
    W25Q64_Write_4Floats(PID_FLASH_ADDR + 16, &pid_flash_buf[4]);

    /* 立即回读验证（排查写入是否真正成功） */
    float verify[4] = {0};
    W25Q64_Read_4Floats(PID_FLASH_ADDR, verify);
    if (verify[0] == PID_FLASH_MAGIC) {
        snprintf(uart_buf, sizeof(uart_buf),
                 "[PID] 已保存到Flash（验证OK，sp_kp=%.4f）\r\n",
                 (double)verify[1]);
        UART_SendString(uart_buf);
    } else {
        /* 原始 hex 辅助诊断 */
        uint8_t *raw = (uint8_t *)verify;
        snprintf(uart_buf, sizeof(uart_buf),
                 "[PID] Flash写入失败！readback=%.4f raw=%02X%02X%02X%02X\r\n",
                 (double)verify[0],
                 raw[0], raw[1], raw[2], raw[3]);
        UART_SendString(uart_buf);
    }
}

static void PID_LoadFromFlash(void) {
    if (!flash_id_ok) {
        UART_SendString("[PID] Flash未连接，使用默认参数\r\n");
        return;
    }
    W25Q64_Read_4Floats(PID_FLASH_ADDR,      &pid_flash_buf[0]);
    W25Q64_Read_4Floats(PID_FLASH_ADDR + 16, &pid_flash_buf[4]);

    /* 始终打印 Flash 中读到的 magic，方便判断是否擦除/写入成功 */
    uint8_t *raw = (uint8_t *)&pid_flash_buf[0];
    snprintf(uart_buf, sizeof(uart_buf),
             "[PID] Flash magic=%.4f raw=%02X%02X%02X%02X\r\n",
             (double)pid_flash_buf[0],
             raw[0], raw[1], raw[2], raw[3]);
    UART_SendString(uart_buf);

    if (pid_flash_buf[0] != PID_FLASH_MAGIC) {
        UART_SendString("[PID] Flash无有效参数，使用默认值\r\n");
        return;
    }
    speed_pid.kp   = pid_flash_buf[1];
    speed_pid.ki   = pid_flash_buf[2];
    speed_pid.kd   = pid_flash_buf[3];
    current_pid.kp = pid_flash_buf[4];
    current_pid.ki = pid_flash_buf[5];
    current_pid.kd = pid_flash_buf[6];
    UART_SendString("[PID] 已从Flash加载参数\r\n");
}

static void handle_pid_cmd(const char *cmd) {
    float val;
    /* 速度外环 */
    if      (sscanf(cmd, "set sp %f", &val) == 1) { speed_pid.kp = val; snprintf(uart_buf, sizeof(uart_buf), "[PID] speed_kp=%.4f\r\n", (double)val); UART_SendString(uart_buf); }
    else if (sscanf(cmd, "set si %f", &val) == 1) { speed_pid.ki = val; snprintf(uart_buf, sizeof(uart_buf), "[PID] speed_ki=%.4f\r\n", (double)val); UART_SendString(uart_buf); }
    else if (sscanf(cmd, "set sd %f", &val) == 1) { speed_pid.kd = val; snprintf(uart_buf, sizeof(uart_buf), "[PID] speed_kd=%.4f\r\n", (double)val); UART_SendString(uart_buf); }
    /* 电流内环 */
    else if (sscanf(cmd, "set cp %f", &val) == 1) { current_pid.kp = val; snprintf(uart_buf, sizeof(uart_buf), "[PID] curr_kp=%.4f\r\n", (double)val); UART_SendString(uart_buf); }
    else if (sscanf(cmd, "set ci %f", &val) == 1) { current_pid.ki = val; snprintf(uart_buf, sizeof(uart_buf), "[PID] curr_ki=%.4f\r\n", (double)val); UART_SendString(uart_buf); }
    else if (sscanf(cmd, "set cd %f", &val) == 1) { current_pid.kd = val; snprintf(uart_buf, sizeof(uart_buf), "[PID] curr_kd=%.4f\r\n", (double)val); UART_SendString(uart_buf); }
    /* 通用操作 */
    else if (strcmp(cmd, "save") == 0) { PID_SaveToFlash(); }
    else if (strcmp(cmd, "load") == 0) { PID_LoadFromFlash(); }
    else if (strcmp(cmd, "show") == 0) {
        snprintf(uart_buf, sizeof(uart_buf),
            "speed:   kp=%.4f ki=%.4f kd=%.4f\r\ncurrent: kp=%.4f ki=%.4f kd=%.4f\r\n",
            (double)speed_pid.kp,   (double)speed_pid.ki,   (double)speed_pid.kd,
            (double)current_pid.kp, (double)current_pid.ki, (double)current_pid.kd);
        UART_SendString(uart_buf);
    }
    else if (strcmp(cmd, "help") == 0) {
        UART_SendString("Commands:\r\n"
                        "  set sp/si/sd <val>  speed kp/ki/kd\r\n"
                        "  set cp/ci/cd <val>  current kp/ki/kd\r\n"
                        "  show   print params\r\n"
                        "  save   write to Flash\r\n"
                        "  load   read from Flash\r\n");
    }
    else if (cmd[0] != '\0') {
        UART_SendString("[PID] unknown cmd, type 'help'\r\n");
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* 必须先配置时钟，再初始化依赖时钟的外设。
   * Motor_Control_Init 配置 TIM6 PSC=83/ARR=999，假设 APB1_TIM_CLK=84MHz。
   * 若在 SystemClock_Config 之前调用，APB1 仍为 HSI=16MHz，
   * TIM6 实际只有 ~190Hz，RPM 读数被放大 5.26×，PID 完全失效。 */
  SystemClock_Config();

  /* USER CODE BEGIN Init */
	UART_Init();
	SPI1_Flash_Init();
	SPI2_FPGA_Init();
	ADC1_Init();
	Motor_Control_Init();   /* TIM6 1kHz + PID 初始化，从此 PID 在中断里自动运行 */
  /* USER CODE END Init */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  /* 初始化串口、I2C 和 OLED */
 // UART_Init();      // 已在 1 处调用，可重复安全调用
	/*
		if no connected the OLED screen, but init it in code,
		execution will be stalled here, pay attention.
		
	*/
  I2C_Init();
  OLED_Init();
  OLED_ShowStr(0, 0, (uint8_t *)"KELI_MOTOR", FontSize6x8, 0);
  UART_SendString("hhhh\r\n");
  /* ============================================================
   * W25Q64FV Flash 测试
   * ============================================================ */

  /* Step 1: 读 ID，验证 SPI 通信是否正常 */
  flash_id = W25Q64_ReadID();
  if (flash_id == 0xEF16) {
      flash_id_ok = 1;
      UART_SendString("[Flash] ID OK: 0xEF16\r\n");
  } else {
      flash_id_ok = 0;
      snprintf(uart_buf, sizeof(uart_buf), "[Flash] ID FAIL: 0x%04X (expected 0xEF16)\r\n", flash_id);
      UART_SendString(uart_buf);
  }

  /* Step 2: 解除写保护（SR1=0x00, SR2=0x00） */
  W25Q64_Unprotect();
  UART_SendString("[Flash] Unprotect done\r\n");

  /* 上电尝试从Flash加载PID参数（Flash未接线时自动跳过） */
  PID_LoadFromFlash();
  UART_SendString("type 'help' for PID commands\r\n");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* PID 控制环已在 TIM6 1kHz 中断里自动运行，主循环只负责监控打印 */

    /* ---- 第一步：先轮询串口 200ms，期间不发任何数据，保证收得到命令 ---- */
    uint8_t  rx_active    = 0;
    uint32_t t0           = HAL_GetTick();
    uint32_t last_rx_tick = 0;   /* 最后一次收到字符的时刻 */
    while (HAL_GetTick() - t0 < 200) {
        char ch;
        if (UART_RecvChar(&ch)) {
            rx_active    = 1;
            last_rx_tick = HAL_GetTick();
            if (ch == '\r' || ch == '\n') {
                if (cmd_len > 0) {
                    cmd_buf[cmd_len] = '\0';
                    handle_pid_cmd(cmd_buf);  /* save会在这里阻塞~400ms */
                    cmd_len      = 0;
                    last_rx_tick = 0;
                }
            } else if (cmd_len < 63) {
                cmd_buf[cmd_len++] = ch;
            }
        }
        /* 50ms 静默超时：串口助手不发换行符时也能自动执行命令 */
        if (cmd_len > 0 && last_rx_tick > 0 &&
            HAL_GetTick() - last_rx_tick >= 50) {
            cmd_buf[cmd_len] = '\0';
            handle_pid_cmd(cmd_buf);
            cmd_len      = 0;
            last_rx_tick = 0;
        }
    }

    /* ---- 第二步：没有命令活动时才打印 RPM / 刷新 OLED（5Hz） ---- */
    if (!rx_active) {
        uint16_t curr_raw = ADC1_ReadChannel(1U);
        snprintf(uart_buf, sizeof(uart_buf),
                 "RPM=%.1f  duty=%4u (%.1f%%)  adc=%4u  curr_raw=%4u  curr=%.1fmA\r\n",
                 (double)g_encoder_rpm,
                 g_pid_duty, (double)g_pid_duty / 40.95,
                 g_adc_val, curr_raw,
                 (double)g_motor_current_mA);
        UART_SendString(uart_buf);

        char oled_buf[22];
        snprintf(oled_buf, sizeof(oled_buf), "RPM:%-7.1f", (double)g_encoder_rpm);
        OLED_ShowStr(0, 2, (uint8_t *)oled_buf, FontSize6x8, 0);
        snprintf(oled_buf, sizeof(oled_buf), "Duty:%-6.1f%%", (double)g_pid_duty / 40.95);
        OLED_ShowStr(0, 4, (uint8_t *)oled_buf, FontSize6x8, 0);
        snprintf(oled_buf, sizeof(oled_buf), "Curr:%-6.1fmA", (double)g_motor_current_mA);
        OLED_ShowStr(0, 6, (uint8_t *)oled_buf, FontSize6x8, 0);
    }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
