#ifndef SPI_REG_H
#define SPI_REG_H

#include "stm32f4xx.h"

/* SPI1 (W25Q64 Flash): PA4=CS, PA5=SCK, PA6=MISO, PA7=MOSI — AF5 */
#define FLASH_CS_LOW()   (GPIOA->BSRR = (1U << (4U + 16U)))
#define FLASH_CS_HIGH()  (GPIOA->BSRR = (1U << 4U))

/* SPI2 (FPGA): PB12=CS, PB13=SCK, PB14=MISO, PB15=MOSI — AF5 */
#define FPGA_CS_LOW()    (GPIOB->BSRR = (1U << (12U + 16U)))
#define FPGA_CS_HIGH()   (GPIOB->BSRR = (1U << 12U))

void    SPI1_Flash_Init(void);
uint8_t SPI1_ReadWriteByte(uint8_t tx_data);

void    SPI2_FPGA_Init(void);
uint8_t SPI2_ReadWriteByte(uint8_t tx_data);

#endif /* SPI_REG_H */
