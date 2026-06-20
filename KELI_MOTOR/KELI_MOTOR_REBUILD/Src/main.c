/* Main program — KELI_MOTOR_REBUILD */

#include "main.h"
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
#include "motor_fsm.h"

extern void     Motor_Control_Init(void);
extern PID_TypeDef speed_pid;
extern PID_TypeDef current_pid;

/* Flash storage: sector 0, magic sentinel to detect valid data */
#define PID_FLASH_ADDR   0x000000UL
#define PID_FLASH_MAGIC  12345.678f

static char    uart_buf[128];
static float   pid_flash_buf[8];   /* [magic, sp_kp, sp_ki, sp_kd, cp_kp, cp_ki, cp_kd, 0] */

static uint16_t flash_id    = 0U;
static uint8_t  flash_id_ok = 0U;

/* ---- Command parser ---- */
static char    cmd_buf[64];
static uint8_t cmd_len = 0U;

/* Parse "set XX <float>" from cmd string. name is a 2-char key like "sp". */
static uint8_t parse_float_arg(const char *cmd, const char *name, float *value)
{
    const char *p = cmd;
    float result = 0.0f, scale = 0.1f;
    uint8_t neg = 0U, has_digit = 0U;

    while (*p == ' ' || *p == '\t') { p++; }
    if (strncmp(p, "set", 3U) != 0) { return 0U; }
    p += 3;
    while (*p == ' ' || *p == '\t') { p++; }
    if (p[0] != name[0] || p[1] != name[1]) { return 0U; }
    p += 2;
    if (*p != ' ' && *p != '\t') { return 0U; }
    while (*p == ' ' || *p == '\t') { p++; }

    if (*p == '-' || *p == '+') { neg = (*p == '-') ? 1U : 0U; p++; }
    while (*p >= '0' && *p <= '9') { result = result * 10.0f + (float)(*p - '0'); has_digit = 1U; p++; }
    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') { result += (float)(*p - '0') * scale; scale *= 0.1f; has_digit = 1U; p++; }
    }
    while (*p != '\0' && ((uint8_t)*p <= 0x20U || (uint8_t)*p == 0x7FU)) { p++; }
    if (!has_digit || *p != '\0') { return 0U; }

    *value = neg ? -result : result;
    return 1U;
}

static void PID_SaveToFlash(void)
{
    if (!flash_id_ok) { UART_SendString("[PID] Flash not connected, save skipped\r\n"); return; }

    pid_flash_buf[0] = PID_FLASH_MAGIC;
    pid_flash_buf[1] = speed_pid.kp;
    pid_flash_buf[2] = speed_pid.ki;
    pid_flash_buf[3] = speed_pid.kd;
    pid_flash_buf[4] = current_pid.kp;
    pid_flash_buf[5] = current_pid.ki;
    pid_flash_buf[6] = current_pid.kd;
    pid_flash_buf[7] = 0.0f;

    uint8_t sr1 = W25Q64_ReadSR1(), sr2 = W25Q64_ReadSR2();
    snprintf(uart_buf, sizeof(uart_buf),
             "[PID] SR1=0x%02X SR2=0x%02X (WEL=%d BP=%d CMP=%d)\r\n",
             sr1, sr2, (sr1 >> 1) & 1, (sr1 >> 2) & 7, (sr2 >> 6) & 1);
    UART_SendString(uart_buf);

    W25Q64_Erase_Sector(PID_FLASH_ADDR);
    W25Q64_Write_4Floats(PID_FLASH_ADDR,       &pid_flash_buf[0]);
    W25Q64_Write_4Floats(PID_FLASH_ADDR + 16U, &pid_flash_buf[4]);

    float verify[4] = {0.0f};
    W25Q64_Read_4Floats(PID_FLASH_ADDR, verify);
    if (verify[0] == PID_FLASH_MAGIC) {
        snprintf(uart_buf, sizeof(uart_buf), "[PID] Saved OK (sp_kp=%.4f)\r\n", (double)verify[1]);
    } else {
        uint8_t *raw = (uint8_t *)verify;
        snprintf(uart_buf, sizeof(uart_buf),
                 "[PID] Save FAILED readback=%.4f raw=%02X%02X%02X%02X\r\n",
                 (double)verify[0], raw[0], raw[1], raw[2], raw[3]);
    }
    UART_SendString(uart_buf);
}

static void PID_LoadFromFlash(void)
{
    if (!flash_id_ok) { UART_SendString("[PID] Flash not connected, using defaults\r\n"); return; }

    W25Q64_Read_4Floats(PID_FLASH_ADDR,       &pid_flash_buf[0]);
    W25Q64_Read_4Floats(PID_FLASH_ADDR + 16U, &pid_flash_buf[4]);

    uint8_t *raw = (uint8_t *)&pid_flash_buf[0];
    snprintf(uart_buf, sizeof(uart_buf),
             "[PID] Flash magic=%.4f raw=%02X%02X%02X%02X\r\n",
             (double)pid_flash_buf[0], raw[0], raw[1], raw[2], raw[3]);
    UART_SendString(uart_buf);

    if (pid_flash_buf[0] != PID_FLASH_MAGIC) {
        UART_SendString("[PID] No valid params in Flash, using defaults\r\n");
        return;
    }
    speed_pid.kp   = pid_flash_buf[1];
    speed_pid.ki   = pid_flash_buf[2];
    speed_pid.kd   = pid_flash_buf[3];
    current_pid.kp = pid_flash_buf[4];
    current_pid.ki = pid_flash_buf[5];
    current_pid.kd = pid_flash_buf[6];
    UART_SendString("[PID] Loaded from Flash\r\n");
}

static void handle_pid_cmd(const char *cmd)
{
    float val;
    if      (parse_float_arg(cmd, "sp", &val)) { speed_pid.kp   = val; PID_Reset(&speed_pid);   snprintf(uart_buf, sizeof(uart_buf), "[PID] speed_kp=%.4f\r\n",  (double)val); UART_SendString(uart_buf); }
    else if (parse_float_arg(cmd, "si", &val)) { speed_pid.ki   = val; PID_Reset(&speed_pid);   snprintf(uart_buf, sizeof(uart_buf), "[PID] speed_ki=%.4f\r\n",  (double)val); UART_SendString(uart_buf); }
    else if (parse_float_arg(cmd, "sd", &val)) { speed_pid.kd   = val; PID_Reset(&speed_pid);   snprintf(uart_buf, sizeof(uart_buf), "[PID] speed_kd=%.4f\r\n",  (double)val); UART_SendString(uart_buf); }
    else if (parse_float_arg(cmd, "cp", &val)) { current_pid.kp = val; PID_Reset(&current_pid); snprintf(uart_buf, sizeof(uart_buf), "[PID] curr_kp=%.4f\r\n",   (double)val); UART_SendString(uart_buf); }
    else if (parse_float_arg(cmd, "ci", &val)) { current_pid.ki = val; PID_Reset(&current_pid); snprintf(uart_buf, sizeof(uart_buf), "[PID] curr_ki=%.4f\r\n",   (double)val); UART_SendString(uart_buf); }
    else if (parse_float_arg(cmd, "cd", &val)) { current_pid.kd = val; PID_Reset(&current_pid); snprintf(uart_buf, sizeof(uart_buf), "[PID] curr_kd=%.4f\r\n",   (double)val); UART_SendString(uart_buf); }
    else if (strcmp(cmd, "save") == 0)  { PID_SaveToFlash(); }
    else if (strcmp(cmd, "load") == 0)  { PID_LoadFromFlash(); }
    else if (strcmp(cmd, "show") == 0)  {
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
        snprintf(uart_buf, sizeof(uart_buf), "[PID] unknown: <%s>\r\n", cmd);
        UART_SendString(uart_buf);
    }
}

int main(void)
{
    HAL_Init();

    /* SystemClock_Config must be called before Motor_Control_Init.
     * Motor_Control_Init sets TIM6 PSC/ARR assuming APB1_TIM_CLK=84MHz.
     * If called before clock config, APB1 is still 16MHz -> TIM6 runs at ~190Hz
     * instead of 1kHz, causing 5x RPM read error and broken PID. */
    SystemClock_Config();

    UART_Init();
    UART_SendString("[REBUILD] boot\r\n");

    SPI1_Flash_Init();
    SPI2_FPGA_Init();
    ADC1_Init();

    /* Force FPGA duty=0 before calibrating current zero.
     * FPGA may hold a non-zero duty from before MCU reset. */
    FPGA_CS_LOW();
    SPI2_ReadWriteByte(0U);
    SPI2_ReadWriteByte(0U);
    FPGA_CS_HIGH();
    HAL_Delay(100U);
    ADC1_CalibrateCurrentZero();

    Motor_Control_Init();

    I2C_Init();
    OLED_Init();
    OLED_ShowStr(0U, 0U, (uint8_t *)"KELI_MOTOR", FontSize6x8, 0U);

    flash_id = W25Q64_ReadID();
    if (flash_id == 0xEF16U) {
        flash_id_ok = 1U;
        UART_SendString("[Flash] ID OK: 0xEF16\r\n");
    } else {
        flash_id_ok = 0U;
        snprintf(uart_buf, sizeof(uart_buf), "[Flash] ID FAIL: 0x%04X\r\n", flash_id);
        UART_SendString(uart_buf);
    }
    W25Q64_Unprotect();
    PID_LoadFromFlash();
    UART_SendString("type 'help' for commands\r\n");

    while (1) {
        /* Poll UART for 200ms; print telemetry only when no input activity */
        uint8_t  rx_active    = 0U;
        uint32_t t0           = HAL_GetTick();
        uint32_t last_rx_tick = 0U;

        while (HAL_GetTick() - t0 < 200U) {
            char ch;
            if (UART_RecvChar(&ch)) {
                rx_active    = 1U;
                last_rx_tick = HAL_GetTick();
                if (ch == '\r' || ch == '\n') {
                    if (cmd_len > 0U) {
                        cmd_buf[cmd_len] = '\0';
                        handle_pid_cmd(cmd_buf);
                        cmd_len      = 0U;
                        last_rx_tick = 0U;
                    }
                } else if (cmd_len < 63U) {
                    cmd_buf[cmd_len++] = ch;
                }
            }
            /* 50ms silence timeout: execute even if terminal omits newline */
            if (cmd_len > 0U && last_rx_tick > 0U &&
                HAL_GetTick() - last_rx_tick >= 50U) {
                cmd_buf[cmd_len] = '\0';
                handle_pid_cmd(cmd_buf);
                cmd_len      = 0U;
                last_rx_tick = 0U;
            }
        }

        if (!rx_active) {
            MotorTelemetry t;
            Motor_GetTelemetry(&t);

            float display_rpm     = (t.duty == 0U || (t.rpm > -0.05f && t.rpm < 0.05f)) ? 0.0f : t.rpm;
            float display_current = (t.current_mA < 0.05f) ? 0.0f : t.current_mA;

            snprintf(uart_buf, sizeof(uart_buf),
                     "[%-5s] set=%.1f setR=%.1f RPM=%.1f duty=%4u (%.1f%%) adc=%4u raw=%4u iset=%.1fmA curr=%.1fmA\r\n",
                     Motor_State_Name(t.state),
                     (double)t.rpm_set, (double)t.rpm_set_ramped, (double)display_rpm,
                     t.duty, (double)t.duty / 40.95,
                     t.pot_adc, t.current_raw,
                     (double)t.current_set_mA, (double)display_current);
            UART_SendString(uart_buf);

            char oled_buf[22];
            snprintf(oled_buf, sizeof(oled_buf), "%-5s RPM:%-5.1f", Motor_State_Name(t.state), (double)display_rpm);
            OLED_ShowStr(0U, 2U, (uint8_t *)oled_buf, FontSize6x8, 0U);
            snprintf(oled_buf, sizeof(oled_buf), "Duty:%-6.1f%%", (double)t.duty / 40.95);
            OLED_ShowStr(0U, 4U, (uint8_t *)oled_buf, FontSize6x8, 0U);
            snprintf(oled_buf, sizeof(oled_buf), "Curr:%-6.1fmA", (double)display_current);
            OLED_ShowStr(0U, 6U, (uint8_t *)oled_buf, FontSize6x8, 0U);
        }
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 8;
    RCC_OscInitStruct.PLL.PLLN            = 168;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ            = 4;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) { Error_Handler(); }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) { Error_Handler(); }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file; (void)line;
}
#endif
