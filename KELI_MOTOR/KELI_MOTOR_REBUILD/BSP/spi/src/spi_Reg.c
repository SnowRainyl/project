#include "spi_Reg.h"

/* SPI1 — W25Q64 Flash, Mode 0, APB2(84MHz)/16 = 5.25MHz */
void SPI1_Flash_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    /* PA4: GPIO output (software CS) */
    GPIOA->MODER   &= ~(3U << (4U * 2U));
    GPIOA->MODER   |=  (1U << (4U * 2U));
    GPIOA->OSPEEDR |=  (3U << (4U * 2U));
    FLASH_CS_HIGH();

    /* PA5=SCK, PA6=MISO, PA7=MOSI — AF5 */
    GPIOA->MODER   &= ~((3U << (5U * 2U)) | (3U << (6U * 2U)) | (3U << (7U * 2U)));
    GPIOA->MODER   |=  ((2U << (5U * 2U)) | (2U << (6U * 2U)) | (2U << (7U * 2U)));
    GPIOA->OSPEEDR |=  ((3U << (5U * 2U)) | (3U << (6U * 2U)) | (3U << (7U * 2U)));
    GPIOA->AFR[0]  &= ~((0xFU << (5U * 4U)) | (0xFU << (6U * 4U)) | (0xFU << (7U * 4U)));
    GPIOA->AFR[0]  |=  ((5U   << (5U * 4U)) | (5U   << (6U * 4U)) | (5U   << (7U * 4U)));

    SPI1->CR1 = 0U;
    SPI1->CR1 |= (3U << SPI_CR1_BR_Pos);   /* BR=011: fPCLK/16 */
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR;
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI1_ReadWriteByte(uint8_t tx_data)
{
    while (!(SPI1->SR & SPI_SR_TXE)) {}
    /* 8-bit pointer write prevents 16-bit write on F4 (would corrupt FIFO) */
    *(volatile uint8_t *)&SPI1->DR = tx_data;
    while (!(SPI1->SR & SPI_SR_RXNE)) {}
    return *(volatile uint8_t *)&SPI1->DR;
}

/* SPI2 — FPGA, Mode 0, APB1(42MHz)/8 = 5.25MHz (same SCK as SPI1) */
void SPI2_FPGA_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    /* PB12: GPIO output (software CS) */
    GPIOB->MODER   &= ~(3U << (12U * 2U));
    GPIOB->MODER   |=  (1U << (12U * 2U));
    GPIOB->OSPEEDR |=  (3U << (12U * 2U));
    FPGA_CS_HIGH();

    /* PB13=SCK, PB14=MISO, PB15=MOSI — AF5 (AFRH: pin-8 offset) */
    GPIOB->MODER   &= ~((3U << (13U * 2U)) | (3U << (14U * 2U)) | (3U << (15U * 2U)));
    GPIOB->MODER   |=  ((2U << (13U * 2U)) | (2U << (14U * 2U)) | (2U << (15U * 2U)));
    GPIOB->OSPEEDR |=  ((3U << (13U * 2U)) | (3U << (14U * 2U)) | (3U << (15U * 2U)));
    GPIOB->AFR[1]  &= ~((0xFU << ((13U - 8U) * 4U)) | (0xFU << ((14U - 8U) * 4U)) | (0xFU << ((15U - 8U) * 4U)));
    GPIOB->AFR[1]  |=  ((5U   << ((13U - 8U) * 4U)) | (5U   << ((14U - 8U) * 4U)) | (5U   << ((15U - 8U) * 4U)));

    SPI2->CR1 = 0U;
    SPI2->CR1 |= (2U << SPI_CR1_BR_Pos);   /* BR=010: fPCLK/8 */
    SPI2->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR;
    SPI2->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI2_ReadWriteByte(uint8_t tx_data)
{
    while (!(SPI2->SR & SPI_SR_TXE)) {}
    *(volatile uint8_t *)&SPI2->DR = tx_data;
    while (!(SPI2->SR & SPI_SR_RXNE)) {}
    return *(volatile uint8_t *)&SPI2->DR;
}
