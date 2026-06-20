#include "OLED_SSD1306.h"
#include "custom_i2c.h"

static void OLED_Write_Byte(uint8_t data, uint8_t cmd_or_data)
{
    /* OLED_Addr=0x78 is the 8-bit form; shift right to get 7-bit address 0x3C */
    i2c_write_memory(OLED_Addr >> 1U, cmd_or_data, data, 1U);
}

static void OLED_SetPos(uint8_t x, uint8_t y)
{
    OLED_Write_Byte((uint8_t)(0xB0U + y),              CmdReg);
    OLED_Write_Byte((uint8_t)(((x & 0xF0U) >> 4U) | 0x10U), CmdReg);
    OLED_Write_Byte((uint8_t)((x & 0x0FU) | 0x01U),   CmdReg);
}

void OLED_Fill(uint8_t fill_data)
{
    uint8_t i, j;
    for (i = 0U; i < 8U; i++) {
        OLED_Write_Byte((uint8_t)(0xB0U + i), CmdReg);
        OLED_Write_Byte(0x10U, CmdReg);
        OLED_Write_Byte(0x00U, CmdReg);
        for (j = 0U; j < OLED_Width; j++) {
            OLED_Write_Byte(fill_data, DataReg);
        }
    }
}

void OLED_LocalFill(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t data)
{
    uint8_t i, j;
    for (i = y0; i <= y1; i++) {
        OLED_SetPos(x0, i);
        for (j = x0; j < x1; j++) {
            OLED_Write_Byte(data, DataReg);
        }
    }
}

void OLED_Init(void)
{
    OLED_Delay_ms(200U);
    OLED_Write_Byte(0xAEU, CmdReg);   /* display off */
    OLED_Write_Byte(0x20U, CmdReg);   /* memory addressing mode */
    OLED_Write_Byte(0x10U, CmdReg);   /* page addressing mode */
    OLED_Write_Byte(0x00U, CmdReg);   /* low column address */
    OLED_Write_Byte(0x10U, CmdReg);   /* high column address */
    OLED_Write_Byte(0x40U, CmdReg);   /* start line = 0 */
    OLED_Write_Byte(0xB0U, CmdReg);   /* page address = 0 */
    OLED_Write_Byte(0x81U, CmdReg);   /* contrast control */
    OLED_Write_Byte(0xFFU, CmdReg);
    OLED_Write_Byte(0xC8U, CmdReg);   /* COM scan direction (flip vertical) */
    OLED_Write_Byte(0xA1U, CmdReg);   /* segment remap (flip horizontal) */
    OLED_Write_Byte(0xA6U, CmdReg);   /* normal display */
    OLED_Write_Byte(0xA8U, CmdReg);   /* multiplex ratio */
    OLED_Write_Byte(0x3FU, CmdReg);   /* 1/64 duty */
    OLED_Write_Byte(0xD3U, CmdReg);   /* display offset */
    OLED_Write_Byte(0x00U, CmdReg);
    OLED_Write_Byte(0xD5U, CmdReg);   /* clock divide / oscillator */
    OLED_Write_Byte(0x80U, CmdReg);
    OLED_Write_Byte(0xD8U, CmdReg);   /* area color mode off */
    OLED_Write_Byte(0x05U, CmdReg);
    OLED_Write_Byte(0xD9U, CmdReg);   /* pre-charge period */
    OLED_Write_Byte(0xF1U, CmdReg);
    OLED_Write_Byte(0xDAU, CmdReg);   /* COM pin configuration */
    OLED_Write_Byte(0x12U, CmdReg);
    OLED_Write_Byte(0xDBU, CmdReg);   /* Vcomh deselect level */
    OLED_Write_Byte(0x30U, CmdReg);
    OLED_Write_Byte(0x8DU, CmdReg);   /* charge pump */
    OLED_Write_Byte(0x14U, CmdReg);
    OLED_Write_Byte(0xA4U, CmdReg);   /* entire display on (resume from RAM) */
#if OLED_INVERSE_COLOR
    OLED_Write_Byte(0xA7U, CmdReg);
#else
    OLED_Write_Byte(0xA6U, CmdReg);
#endif
    OLED_Write_Byte(0xAFU, CmdReg);   /* display on */
    OLED_Fill(Dark);
    OLED_Delay_ms(100U);
}

void OLED_ShowChar(uint8_t x, uint8_t y, uint8_t ch, uint8_t font_size, uint8_t invert)
{
    uint8_t c = ch - 32U;   /* font arrays start at ASCII 32 (space) */
    uint8_t i;

    if (x > 127U) { x = 0U; y += 2U; }

    switch (font_size) {
    case FontSize6x8:
        OLED_SetPos(x, y);
        for (i = 0U; i < 6U; i++) {
            uint8_t b = F6X8[c * 6U + i];
            OLED_Write_Byte(invert ? ~b : b, DataReg);
        }
        break;
    case FontSize8x16:
        OLED_SetPos(x, y);
        for (i = 0U; i < 8U; i++) {
            uint8_t b = F8X16[c * 16U + i];
            OLED_Write_Byte(invert ? ~b : b, DataReg);
        }
        OLED_SetPos(x, (uint8_t)(y + 1U));
        for (i = 0U; i < 8U; i++) {
            uint8_t b = F8X16[c * 16U + i + 8U];
            OLED_Write_Byte(invert ? ~b : b, DataReg);
        }
        break;
    default:
        break;
    }
}

void OLED_ShowStr(uint8_t x, uint8_t y, uint8_t *str, uint8_t font_size, uint8_t invert)
{
    uint8_t j = 0U;
    uint8_t step = (font_size == FontSize6x8) ? 6U : 8U;

    while (str[j] != '\0') {
        OLED_ShowChar(x, y, str[j], font_size, invert);
        x += step;
        if (font_size == FontSize6x8  && x > 122U) { x = 0U; y++; }
        if (font_size == FontSize8x16 && x > 120U) { x = 0U; y++; }
        j++;
    }
}
