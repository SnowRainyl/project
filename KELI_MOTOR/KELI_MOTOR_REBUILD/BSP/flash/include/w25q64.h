#ifndef W25Q64_H
#define W25Q64_H

#include "stm32f4xx.h"
#include "spi_Reg.h"

/* W25Q64FV instruction set (Datasheet Table 1) */
#define W25X_WriteEnable        0x06U
#define W25X_WriteDisable       0x04U
#define W25X_ReadStatusReg1     0x05U   /* bit0=BUSY, bit1=WEL */
#define W25X_ReadStatusReg2     0x35U   /* bit6=CMP */
#define W25X_WriteStatusReg     0x01U   /* 2 bytes: SR1, SR2 */
#define W25X_ReadData           0x03U
#define W25X_PageProgram        0x02U   /* 256 bytes/page, tPP max 3ms */
#define W25X_SectorErase        0x20U   /* 4KB, tSE max 400ms */
#define W25X_ManufactDeviceID   0x90U   /* returns 0xEF16 for W25Q64 */
#define W25X_JedecDeviceID      0x9FU

#define W25Q64_SR1_BUSY         0x01U
#define W25Q64_SR1_WEL          0x02U

uint16_t W25Q64_ReadID(void);
void     W25Q64_Unprotect(void);
void     W25Q64_Erase_Sector(uint32_t addr);
void     W25Q64_Write_4Floats(uint32_t addr, float *pf);
void     W25Q64_Read_4Floats(uint32_t addr, float *pf);
uint8_t  W25Q64_ReadSR1(void);
uint8_t  W25Q64_ReadSR2(void);

#endif /* W25Q64_H */
