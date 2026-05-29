#include "spi_Reg.h"

void SPI1_Flash_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;

    GPIOA->MODER &= ~(3U << (4U * 2U));
    GPIOA->MODER |=  (1U << (4U * 2U));
    GPIOA->OSPEEDR |= (3U << (4U * 2U));
    FLASH_CS_HIGH();

    GPIOA->MODER &= ~((3U << (5U * 2U)) | (3U << (6U * 2U)) | (3U << (7U * 2U)));
    GPIOA->MODER |=  ((2U << (5U * 2U)) | (2U << (6U * 2U)) | (2U << (7U * 2U)));
    GPIOA->OSPEEDR |= ((3U << (5U * 2U)) | (3U << (6U * 2U)) | (3U << (7U * 2U)));

    GPIOA->AFR[0] &= ~((0xFU << (5U * 4U)) | (0xFU << (6U * 4U)) | (0xFU << (7U * 4U)));
    GPIOA->AFR[0] |=  ((5U   << (5U * 4U)) | (5U   << (6U * 4U)) | (5U   << (7U * 4U)));

    SPI1->CR1 = 0U;
    SPI1->CR1 |= (3U << SPI_CR1_BR_Pos);
    SPI1->CR1 &= ~(SPI_CR1_CPHA | SPI_CR1_CPOL | SPI_CR1_DFF | SPI_CR1_LSBFIRST);
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_MSTR;
    SPI1->CR1 |= SPI_CR1_SPE;
}

uint8_t SPI1_ReadWriteByte(uint8_t tx_data)
{
    while ((SPI1->SR & SPI_SR_TXE) == 0U) {
    }

    *(volatile uint8_t *)&SPI1->DR = tx_data;

    while ((SPI1->SR & SPI_SR_RXNE) == 0U) {
    }

    return *(volatile uint8_t *)&SPI1->DR;
}
