#ifndef W25Q64_H
#define W25Q64_H

#include <stdint.h>
#include "spi_Reg.h"

#define W25X_ReadStatusReg1     0x05U
#define W25X_ReadStatusReg2     0x35U
#define W25X_ManufactDeviceID   0x90U
#define W25X_JedecDeviceID      0x9FU

#define W25Q64_SR1_BUSY         0x01U
#define W25Q64_SR1_WEL          0x02U

uint16_t W25Q64_ReadID(void);
uint32_t W25Q64_ReadJEDECID(void);
uint8_t W25Q64_ReadSR1(void);
uint8_t W25Q64_ReadSR2(void);

#endif /* W25Q64_H */
