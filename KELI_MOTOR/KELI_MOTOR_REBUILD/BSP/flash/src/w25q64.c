#include "w25q64.h"

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

uint16_t W25Q64_ReadID(void)
{
    uint16_t id;

    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_ManufactDeviceID);
    SPI1_ReadWriteByte(0x00U);
    SPI1_ReadWriteByte(0x00U);
    SPI1_ReadWriteByte(0x00U);
    id  = (uint16_t)SPI1_ReadWriteByte(0xFFU) << 8;
    id |= SPI1_ReadWriteByte(0xFFU);
    FLASH_CS_HIGH();

    return id;
}

uint32_t W25Q64_ReadJEDECID(void)
{
    uint32_t id;

    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_JedecDeviceID);
    id  = (uint32_t)SPI1_ReadWriteByte(0xFFU) << 16;
    id |= (uint32_t)SPI1_ReadWriteByte(0xFFU) << 8;
    id |= SPI1_ReadWriteByte(0xFFU);
    FLASH_CS_HIGH();

    return id;
}
