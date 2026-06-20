#include "w25q64.h"
#include "stm32f4xx_hal.h"

uint8_t W25Q64_ReadSR1(void)
{
    uint8_t sr;
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_ReadStatusReg1);
    sr = SPI1_ReadWriteByte(0xFFU);
    FLASH_CS_HIGH();
    return sr;
}

uint8_t W25Q64_ReadSR2(void)
{
    uint8_t sr;
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_ReadStatusReg2);
    sr = SPI1_ReadWriteByte(0xFFU);
    FLASH_CS_HIGH();
    return sr;
}

/* Poll BUSY until clear; timeout covers 400ms sector-erase worst case */
static void W25Q64_Wait_Busy(void)
{
    uint32_t retry = 200000U;
    while ((W25Q64_ReadSR1() & W25Q64_SR1_BUSY) != 0U) {
        if (--retry == 0U) break;
    }
}

/* tSHSL: CS-high to next CS-low must be >= 50ns; HAL_Delay(1) gives safe margin */
static void W25Q64_Write_Enable(void)
{
    HAL_Delay(1);
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_WriteEnable);
    FLASH_CS_HIGH();
    HAL_Delay(1);
}

/* Returns 0xEF16 for W25Q64FV */
uint16_t W25Q64_ReadID(void)
{
    uint16_t id;
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_ManufactDeviceID);
    SPI1_ReadWriteByte(0x00U);   /* A23-A16 */
    SPI1_ReadWriteByte(0x00U);   /* A15-A8  */
    SPI1_ReadWriteByte(0x00U);   /* A7-A0   */
    id  = (uint16_t)SPI1_ReadWriteByte(0xFFU) << 8;
    id |= SPI1_ReadWriteByte(0xFFU);
    FLASH_CS_HIGH();
    return id;
}

/* Clear all block-protect bits in SR1/SR2 so the whole chip is writable */
void W25Q64_Unprotect(void)
{
    W25Q64_Wait_Busy();
    W25Q64_Write_Enable();
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_WriteStatusReg);
    SPI1_ReadWriteByte(0x00U);   /* SR1: clear BP0-BP4, SRP0, TB, SEC */
    SPI1_ReadWriteByte(0x00U);   /* SR2: clear QE, CMP, LB1-3, SRP1 */
    FLASH_CS_HIGH();
    W25Q64_Wait_Busy();          /* tWSR max 15ms */
}

/* Erase 4KB sector containing addr; addr should be 4KB-aligned (0xN000) */
void W25Q64_Erase_Sector(uint32_t addr)
{
    W25Q64_Wait_Busy();
    W25Q64_Write_Enable();
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_SectorErase);
    SPI1_ReadWriteByte((uint8_t)(addr >> 16U));
    SPI1_ReadWriteByte((uint8_t)(addr >> 8U));
    SPI1_ReadWriteByte((uint8_t)(addr));
    FLASH_CS_HIGH();
    HAL_Delay(1);
    W25Q64_Wait_Busy();          /* tSE max 400ms */
}

/* Write 4 floats (16 bytes). Target sector must be erased first. */
void W25Q64_Write_4Floats(uint32_t addr, float *pf)
{
    uint8_t *p = (uint8_t *)pf;
    uint8_t  i;

    W25Q64_Wait_Busy();
    W25Q64_Write_Enable();
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_PageProgram);
    SPI1_ReadWriteByte((uint8_t)(addr >> 16U));
    SPI1_ReadWriteByte((uint8_t)(addr >> 8U));
    SPI1_ReadWriteByte((uint8_t)(addr));
    for (i = 0U; i < (uint8_t)(sizeof(float) * 4U); i++) {
        SPI1_ReadWriteByte(p[i]);
    }
    FLASH_CS_HIGH();
    HAL_Delay(1);
    W25Q64_Wait_Busy();          /* tPP max 3ms */
}

void W25Q64_Read_4Floats(uint32_t addr, float *pf)
{
    uint8_t *p = (uint8_t *)pf;
    uint8_t  i;

    W25Q64_Wait_Busy();
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_ReadData);
    SPI1_ReadWriteByte((uint8_t)(addr >> 16U));
    SPI1_ReadWriteByte((uint8_t)(addr >> 8U));
    SPI1_ReadWriteByte((uint8_t)(addr));
    for (i = 0U; i < (uint8_t)(sizeof(float) * 4U); i++) {
        p[i] = SPI1_ReadWriteByte(0xFFU);
    }
    FLASH_CS_HIGH();
}
