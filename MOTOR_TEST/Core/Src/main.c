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
#include "uart.h"
#include "custom_i2c.h"  /* I2C + OLED 显示接口 */
#include "OLED_SSD1306.h"
#include "spi_Reg.h"
#include "w25q64.h"
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

/* Flash 测试结果，可在 Watch 窗口直接观察 */
uint16_t flash_id    = 0;
uint8_t  flash_id_ok = 0;   /* 1 = ID 正确 (0xEF16) */

static const float write_data[4] = {3.1415f, -2.718f, 100.5f, 0.001f};
static float       read_data[4]  = {0.0f, 0.0f, 0.0f, 0.0f};

#define FLASH_TEST_ADDR   0x001000UL   /* 测试扇区：第2扇区，避免影响扇区0 */
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

  /* USER CODE BEGIN Init */
	UART_Init();
	SPI1_Flash_Init();
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  /* USER CODE BEGIN 2 */
  /* 初始化串口、I2C 和 OLED */
  UART_Init();      // 已在 1 处调用，可重复安全调用
  I2C_Init();
  OLED_Init();
  OLED_ShowChar(0, 0, 'A', FontSize8x16, 1);
  OLED_ShowStr(0, 2, (uint8_t *)"yuqitest", FontSize6x8, 0);

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

  /* Step 3: 擦除测试扇区（4KB @ 0x001000），tSE max 400ms */
  W25Q64_Erase_Sector(FLASH_TEST_ADDR);
  UART_SendString("[Flash] Erase done\r\n");

  /* Step 4: 写入4个float */
  W25Q64_Write_4Floats(FLASH_TEST_ADDR, (float *)write_data);
  UART_SendString("[Flash] Write done\r\n");

  /* Step 5: 读回并通过 UART 打印 */
  W25Q64_Read_4Floats(FLASH_TEST_ADDR, read_data);
  snprintf(uart_buf, sizeof(uart_buf),
           "[Flash] Read: %.4f  %.4f  %.4f  %.4f\r\n",
           (double)read_data[0], (double)read_data[1],
           (double)read_data[2], (double)read_data[3]);
  UART_SendString(uart_buf);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    flash_id = W25Q64_ReadID();
    snprintf(uart_buf, sizeof(uart_buf), "flash_id=0x%04X id_ok=%d\r\n",
             flash_id, flash_id_ok);
    UART_SendString(uart_buf);
    HAL_Delay(1000);
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
