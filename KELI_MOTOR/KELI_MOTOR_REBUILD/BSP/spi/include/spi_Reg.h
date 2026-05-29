#ifndef SPI_REG_H
#define SPI_REG_H

#include <stdint.h>
#include "stm32f4xx.h"

/* W25Q64 on SPI1: PA4=NSS/CS, PA5=SCK, PA6=MISO, PA7=MOSI */
#define FLASH_CS_LOW()      (GPIOA->BSRR = (1U << (4U + 16U)))
#define FLASH_CS_HIGH()     (GPIOA->BSRR = (1U << 4U))

void SPI1_Flash_Init(void);
uint8_t SPI1_ReadWriteByte(uint8_t tx_data);

#endif /* SPI_REG_H */
