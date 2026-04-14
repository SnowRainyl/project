#include "spi_Reg.h"
#include "stm32f4xx.h"
/* =========================================================================
 * SPI1 (NOR Flash) 
 * ========================================================================= */
void SPI1_Flash_Init(void) {
    // 1. 开启时钟：GPIOA (AHB1) 和 SPI1 (APB2)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    // 2. 配置 PA4 作为 FLASH_CS (通用推挽输出)
    GPIOA->MODER &= ~(3U << (4 * 2));   // 清零
    GPIOA->MODER |=  (1U << (4 * 2));   // 01: 输出模式
    GPIOA->OSPEEDR |= (3U << (4 * 2));  // 11: 高速
    FLASH_CS_HIGH();                    // 初始化拉高，取消选中

    // 3. 配置 PA5(SCK), PA6(MISO), PA7(MOSI) 为复用功能 (AF5)
    GPIOA->MODER &= ~((3U << (5 * 2)) | (3U << (6 * 2)) | (3U << (7 * 2)));
    GPIOA->MODER |=  ((2U << (5 * 2)) | (2U << (6 * 2)) | (2U << (7 * 2))); // 10: 复用模式
    
    GPIOA->OSPEEDR |= ((3U << (5 * 2)) | (3U << (6 * 2)) | (3U << (7 * 2))); // 11: 极高速
    
    // 配置 AFRL 寄存器为 AF5 (0x05)
    GPIOA->AFR[0] &= ~((0xFU << (5 * 4)) | (0xFU << (6 * 4)) | (0xFU << (7 * 4)));
    GPIOA->AFR[0] |=  ((0x5U << (5 * 4)) | (0x5U << (6 * 4)) | (0x5U << (7 * 4)));

    // 4. 配置 SPI1 CR1 寄存器
    SPI1->CR1 = 0; // 重置寄存器

    // 波特率配置：APB2 默认最大 84MHz。这里分频 16，得到 5.25MHz，适合初期调试稳定通信。
    // BR[2:0] = 011 -> fPCLK/16
    SPI1->CR1 |= (3U << 3); 

    // SPI 模式配置：Mode 0 (CPOL=0, CPHA=0)。Flash 通常支持 Mode 0 或 Mode 3。
    SPI1->CR1 &= ~(SPI_CR1_CPHA | SPI_CR1_CPOL);

    // 基础配置：8 位数据帧 (DFF=0)，MSB 优先 (LSBFIRST=0)
    SPI1->CR1 &= ~SPI_CR1_DFF;
    SPI1->CR1 &= ~SPI_CR1_LSBFIRST;

    // 软件管理 NSS (SSM=1, SSI=1)，并设置为主机模式 (MSTR=1)
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR;

    // 5. 使能 SPI1
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI1_ReadWriteByte(uint8_t tx_data) {
    // 等待发送缓冲区为空
    while (!(SPI1->SR & SPI_SR_TXE));
    
    // 强制转换为 8 位指针进行写入，防止 16 位写入导致的 F4 硬件怪异行为
    *(volatile uint8_t *)&SPI1->DR = tx_data;
    
    // 等待接收缓冲区非空
    while (!(SPI1->SR & SPI_SR_RXNE));
    
    // 读取收到的数据并返回
    return *(volatile uint8_t *)&SPI1->DR;
}

/* =========================================================================
 * SPI2 (FPGA) 
 * ========================================================================= */
void SPI2_FPGA_Init(void) {
    // 1. 开启时钟：GPIOB (AHB1) 和 SPI2 (APB1)
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    // 2. 配置 PB12 作为 FPGA_CS (通用推挽输出)
    GPIOB->MODER &= ~(3U << (12 * 2));  // 清零
    GPIOB->MODER |=  (1U << (12 * 2));  // 01: 输出模式
    GPIOB->OSPEEDR |= (3U << (12 * 2)); // 11: 高速
    FPGA_CS_HIGH();                     // 初始化拉高，取消选中

    // 3. 配置 PB13(SCK), PB14(MISO), PB15(MOSI) 为复用功能 (AF5)
    GPIOB->MODER &= ~((3U << (13 * 2)) | (3U << (14 * 2)) | (3U << (15 * 2)));
    GPIOB->MODER |=  ((2U << (13 * 2)) | (2U << (14 * 2)) | (2U << (15 * 2))); // 10: 复用模式
    
    GPIOB->OSPEEDR |= ((3U << (13 * 2)) | (3U << (14 * 2)) | (3U << (15 * 2))); // 11: 极高速
    
    // 配置 AFRH 寄存器 (注意：引脚 13-15 位于 AFRH 也就是 AFR[1]) 为 AF5 (0x05)
    GPIOB->AFR[1] &= ~((0xFU << ((13 - 8) * 4)) | (0xFU << ((14 - 8) * 4)) | (0xFU << ((15 - 8) * 4)));
    GPIOB->AFR[1] |=  ((0x5U << ((13 - 8) * 4)) | (0x5U << ((14 - 8) * 4)) | (0x5U << ((15 - 8) * 4)));

    // 4. 配置 SPI2 CR1 寄存器
    SPI2->CR1 = 0;

    // 波特率配置：APB1 默认最大 42MHz。这里分频 8，得到 5.25MHz。
    // 这样设置，SPI1 和 SPI2 的物理时钟频率就一样了，方便使用逻辑分析仪抓包对比。
    // BR[2:0] = 010 -> fPCLK/8
    SPI2->CR1 |= (2U << 3); 

    // 假设 FPGA 端也配置为 Mode 0
    SPI2->CR1 &= ~(SPI_CR1_CPHA | SPI_CR1_CPOL);
    SPI2->CR1 &= ~SPI_CR1_DFF;
    SPI2->CR1 &= ~SPI_CR1_LSBFIRST;

    // 软件管理 NSS，设置为主机模式
    SPI2->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR;

    // 5. 使能 SPI2
    SPI2->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI2_ReadWriteByte(uint8_t tx_data) {
    // 等待发送缓冲区为空
    while (!(SPI2->SR & SPI_SR_TXE));
    
    // 发送数据
    *(volatile uint8_t *)&SPI2->DR = tx_data;
    
    // 等待接收缓冲区非空
    while (!(SPI2->SR & SPI_SR_RXNE));
    
    // 读取数据
    return *(volatile uint8_t *)&SPI2->DR;
}