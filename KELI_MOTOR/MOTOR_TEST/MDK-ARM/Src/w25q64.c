#include "w25q64.h"
#include "stm32f4xx_hal.h"

/* ========================================================================
 * 内部辅助函数
 * ======================================================================== */

/*
 * 读状态寄存器1 (指令 0x05)
 * bit0 BUSY : 1=器件正在执行内部操作，0=就绪
 * bit1 WEL  : 写使能锁存，WriteEnable后为1，写/擦完成后自动清0
 */
uint8_t W25Q64_ReadSR1(void) {
    uint8_t sr;
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_ReadStatusReg1);
    sr = SPI1_ReadWriteByte(0xFF);
    FLASH_CS_HIGH();
    return sr;
}

uint8_t W25Q64_ReadSR2(void) {
    uint8_t sr;
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_ReadStatusReg2);   /* 0x35 */
    sr = SPI1_ReadWriteByte(0xFF);
    FLASH_CS_HIGH();
    return sr;
}

/*
 * 等待内部操作完成，轮询 BUSY 位直到为0
 * 使用计数超时防止死循环（200000次约覆盖 400ms 扇区擦除最坏情况）
 */
static void W25Q64_Wait_Busy(void) {
    uint32_t retry = 200000;
    while ((W25Q64_ReadSR1() & W25Q64_SR1_BUSY) != 0) {
        if (--retry == 0) break;
    }
}

/*
 * 写使能 (指令 0x06)
 * 必须在器件就绪(BUSY=0)后发送，否则指令被忽略、WEL不会置1
 * 正确顺序：Wait_Busy → Write_Enable → 写/擦/写SR 指令
 */
static void W25Q64_Write_Enable(void) {
    HAL_Delay(1);   /* tSHSL: 上一事务结束 → 本次 CS LOW 间隔 >= 100ns */
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_WriteEnable);
    FLASH_CS_HIGH();
    HAL_Delay(1);   /* tSHSL: WREN 结束 → 下一命令 CS LOW 间隔 >= 100ns */
}

/* ========================================================================
 * 公开接口
 * ======================================================================== */

/*
 * 读厂商/设备 ID (指令 0x90)
 * 协议：CMD(0x90) → 哑地址 A23-A16(0x00) → A15-A8(0x00) → A7-A0(0x00)
 *        → 读 Manufacturer ID(0xEF) → 读 Device ID(0x16)
 * 返回值：0xEF16 (W25Q64FV)
 */
uint16_t W25Q64_ReadID(void) {
    uint16_t id = 0;
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_ManufactDeviceID); /* 0x90 */
    SPI1_ReadWriteByte(0x00);                  /* A23-A16 */
    SPI1_ReadWriteByte(0x00);                  /* A15-A8  */
    SPI1_ReadWriteByte(0x00);                  /* A7-A0   */
    id  = (uint16_t)SPI1_ReadWriteByte(0xFF) << 8; /* Manufacturer: 0xEF */
    id |= SPI1_ReadWriteByte(0xFF);                /* Device ID:    0x16 */
    FLASH_CS_HIGH();
    return id;
}

/*
 * 解除全片写保护 (指令 0x01)
 * 将 SR1 和 SR2 全部写0：BP0-BP4=0 且 CMP=0 → 全部扇区可读写
 * tWSR max 15ms，Wait_Busy等待完成
 */
void W25Q64_Unprotect(void) {
    W25Q64_Wait_Busy();        /* 先确认器件就绪 */
    W25Q64_Write_Enable();     /* WEL=1 */
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_WriteStatusReg); /* 0x01 */
    SPI1_ReadWriteByte(0x00);  /* SR1: 清除 BP0-BP4, SRP0, TB, SEC */
    SPI1_ReadWriteByte(0x00);  /* SR2: 清除 QE, CMP, LB1-3, SRP1 */
    FLASH_CS_HIGH();
    W25Q64_Wait_Busy();        /* tWSR max 15ms */
}

/*
 * 扇区擦除 4KB (指令 0x20)
 * addr：目标地址，芯片自动对齐到所在4KB扇区起始处
 *       建议传入对齐地址：0x000000, 0x001000, 0x002000 ...
 * tSE：典型 60ms，最大 400ms
 */
void W25Q64_Erase_Sector(uint32_t addr) {
    W25Q64_Wait_Busy();        /* 确保器件就绪，否则 WE 会被忽略 */
    W25Q64_Write_Enable();     /* WEL=1 */
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_SectorErase);
    SPI1_ReadWriteByte((uint8_t)(addr >> 16)); /* A23-A16 */
    SPI1_ReadWriteByte((uint8_t)(addr >> 8));  /* A15-A8  */
    SPI1_ReadWriteByte((uint8_t)(addr));       /* A7-A0   */
    FLASH_CS_HIGH();
    HAL_Delay(1);              /* tSHSL: Erase→ReadSR 间隔 ≥ 50ns，加 1ms 留足余量 */
    W25Q64_Wait_Busy();        /* 等待擦除完成，max 400ms */
}

/*
 * 写入 4 个 float（16字节）(指令 0x02 Page Program)
 * 前提：目标地址必须已擦除（全0xFF），16字节范围不跨256字节页边界
 * tPP：典型 0.7ms，最大 3ms
 */
void W25Q64_Write_4Floats(uint32_t addr, float *pf) {
    uint8_t *p = (uint8_t *)pf;
    uint8_t  i;

    W25Q64_Wait_Busy();        /* 确保器件就绪 */
    W25Q64_Write_Enable();     /* WEL=1 */
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_PageProgram);
    SPI1_ReadWriteByte((uint8_t)(addr >> 16));
    SPI1_ReadWriteByte((uint8_t)(addr >> 8));
    SPI1_ReadWriteByte((uint8_t)(addr));
    for (i = 0; i < (uint8_t)(sizeof(float) * 4); i++) {
        SPI1_ReadWriteByte(p[i]);
    }
    FLASH_CS_HIGH();
    HAL_Delay(1);              /* tSHSL: PageProgram→ReadSR 间隔 ≥ 50ns */
    W25Q64_Wait_Busy();        /* 等待页编程完成，max 3ms */
}

/*
 * 读取 4 个 float（16字节）(指令 0x03 Read Data)
 * addr：读取起始地址
 * pf  ：接收数组指针
 */
void W25Q64_Read_4Floats(uint32_t addr, float *pf) {
    uint8_t *p = (uint8_t *)pf;
    uint8_t  i;

    W25Q64_Wait_Busy();
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_ReadData);
    SPI1_ReadWriteByte((uint8_t)(addr >> 16));
    SPI1_ReadWriteByte((uint8_t)(addr >> 8));
    SPI1_ReadWriteByte((uint8_t)(addr));
    for (i = 0; i < (uint8_t)(sizeof(float) * 4); i++) {
        p[i] = SPI1_ReadWriteByte(0xFF);
    }
    FLASH_CS_HIGH();
}
