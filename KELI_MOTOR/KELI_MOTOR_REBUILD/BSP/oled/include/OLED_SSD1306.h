#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "FontLib.h"

#define OLED_USING_HARDWARE_I2C  1
#define OLED_USING_SOFTWARE_I2C  0

#define OLED_INVERSE_COLOR  0

#define OLED_Addr    0x78U   /* 8-bit address (bit0=R/W); 7-bit = 0x3C */
#define CmdReg       0x00U
#define DataReg      0x40U
#define OLED_Width   128U
#define OLED_High    64U
#define Bright       0xFFU
#define Dark         0x00U
#define FontSize6x8  1
#define FontSize8x16 2

#define OLED_Delay_ms(ms)  HAL_Delay(ms)

void OLED_Fill(uint8_t fill_data);
void OLED_LocalFill(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t data);
void OLED_Init(void);
void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t ch, uint8_t font_size, uint8_t invert);
void OLED_ShowStr(uint8_t x, uint8_t y, uint8_t *str, uint8_t font_size, uint8_t invert);

#ifdef __cplusplus
}
#endif

#endif /* OLED_SSD1306_H */
