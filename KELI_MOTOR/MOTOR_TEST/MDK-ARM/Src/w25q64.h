#ifndef __W25Q64_H
#define __W25Q64_H

#include "stm32f4xx.h"
#include "spi_Reg.h"

/* ================== W25Q64FV 指令集 (Datasheet Table 1) ================== */
#define W25X_WriteEnable        0x06  /* 写使能，执行写/擦/写SR前必须先发 */
#define W25X_WriteDisable       0x04  /* 写禁止 */
#define W25X_ReadStatusReg1     0x05  /* 读状态寄存器1 (含 BUSY/WEL/BP 位) */
#define W25X_ReadStatusReg2     0x35  /* 读状态寄存器2 (含 QE/CMP 位) */
#define W25X_WriteStatusReg     0x01  /* 写状态寄存器，需先 WriteEnable */
                                      /*   发两字节：SR1_byte, SR2_byte  */
#define W25X_ReadData           0x03  /* 标准读数据，fC max 50MHz */
#define W25X_FastReadData       0x0B  /* 快速读，fC max 104MHz，需1字节dummy */
#define W25X_PageProgram        0x02  /* 页编程，每页256字节，tPP max 3ms */
#define W25X_SectorErase        0x20  /* 扇区擦除 4KB，tSE max 400ms */
#define W25X_BlockErase32       0x52  /* 块擦除 32KB，tBE1 max 800ms */
#define W25X_BlockErase64       0xD8  /* 块擦除 64KB，tBE2 max 2000ms */
#define W25X_ChipErase          0xC7  /* 全片擦除，tCE max 200s */
#define W25X_PowerDown          0xB9  /* 掉电模式，tDP max 3us */
#define W25X_ReleasePowerDown   0xAB  /* 唤醒，tRES1 max 3us */
#define W25X_ManufactDeviceID   0x90  /* 读厂商/设备ID，返回 0xEF(MFR) 0x16(Dev) */
#define W25X_JedecDeviceID      0x9F  /* 读JEDEC ID，返回 0xEF 0x40 0x17 */

/* SR1 位掩码 */
#define W25Q64_SR1_BUSY  0x01   /* bit0: 1=器件忙，0=就绪 */
#define W25Q64_SR1_WEL   0x02   /* bit1: 写使能锁存，1=允许写/擦 */

/* ================== 函数声明 ================== */

/* 读厂商+设备ID，正常返回 0xEF16 */
uint16_t W25Q64_ReadID(void);

/* 解除全片写保护 (SR1=0x00, SR2=0x00) */
void W25Q64_Unprotect(void);

/* 扇区擦除 4KB，addr 建议对齐到 0x1000 边界 */
void W25Q64_Erase_Sector(uint32_t addr);

/* 写入4个float (16字节)，写前须已擦除对应扇区 */
void W25Q64_Write_4Floats(uint32_t addr, float *pf);

/* 读取4个float (16字节) */
void W25Q64_Read_4Floats(uint32_t addr, float *pf);

/* 读状态寄存器1 (BUSY=bit0, WEL=bit1, BP[2:0]=bit4:2) */
uint8_t W25Q64_ReadSR1(void);

/* 读状态寄存器2 (SRP1=bit0, QE=bit1, LB[1:3]=bit2:4, CMP=bit6) */
uint8_t W25Q64_ReadSR2(void);

#endif /* __W25Q64_H */
