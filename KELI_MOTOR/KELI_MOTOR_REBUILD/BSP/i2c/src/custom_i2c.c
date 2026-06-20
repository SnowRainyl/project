#include "custom_i2c.h"
#include "stm32f4xx.h"

#define I2C_TIMEOUT  10000U

void I2C_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    /* PB6=SCL, PB7=SDA — AF4, open-drain, pull-up, high speed */
    GPIOB->MODER   &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOB->MODER   |=  (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);
    GPIOB->OTYPER  |=  (GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);
    GPIOB->OSPEEDR |=  (GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7);
    GPIOB->PUPDR   &= ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7);
    GPIOB->PUPDR   |=  (GPIO_PUPDR_PUPD6_0 | GPIO_PUPDR_PUPD7_0);
    GPIOB->AFR[0]  &= ~((0xFU << (6U * 4U)) | (0xFU << (7U * 4U)));
    GPIOB->AFR[0]  |=  ((4U   << (6U * 4U)) | (4U   << (7U * 4U)));

    /* APB1=42MHz, standard mode 100kHz: CCR=210, TRISE=43 */
    I2C1->CR2   = 42U;
    I2C1->CCR   = 210U;
    I2C1->TRISE = 43U;
    I2C1->CR1  |= I2C_CR1_PE;
}

void i2c_write_memory(uint8_t slav_add, uint8_t memadd, uint8_t data, uint8_t length)
{
    uint8_t send_arr[2];
    uint32_t t;
    (void)length;   /* only single-byte data supported */

    send_arr[0] = memadd;
    send_arr[1] = data;

    /* Clear residual error flags before starting a new transaction */
    I2C1->SR1 &= ~(I2C_SR1_AF | I2C_SR1_ARLO | I2C_SR1_BERR | I2C_SR1_OVR);

    t = I2C_TIMEOUT;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t) {}
    if (!t) { return; }

    I2C1->CR1 |= I2C_CR1_START;
    t = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_SB) && --t) {}
    if (!t) { I2C1->CR1 |= I2C_CR1_STOP; return; }

    I2C1->DR = (uint8_t)(slav_add << 1U);
    t = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_ADDR) && --t) {}
    if (!t) {
        I2C1->SR1 &= ~I2C_SR1_AF;
        I2C1->CR1 |= I2C_CR1_STOP;
        return;
    }

    (void)I2C1->SR1;   /* clear ADDR by reading SR1 then SR2 */
    (void)I2C1->SR2;

    for (int i = 0; i < 2; i++) {
        t = I2C_TIMEOUT;
        while (!(I2C1->SR1 & I2C_SR1_TXE) && --t) {}
        if (!t) { I2C1->CR1 |= I2C_CR1_STOP; return; }
        I2C1->DR = send_arr[i];
    }

    t = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_BTF) && --t) {}

    I2C1->CR1 |= I2C_CR1_STOP;
}
