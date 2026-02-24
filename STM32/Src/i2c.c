/*
 * i2c.c — I2C + OLED SSD1306显示模块实现
 *
 * 硬件：0.96寸 SSD1306 OLED（4针IIC，工作电压3.3V）
 * 引脚：PB6(SCL)，PB7(SDA)
 * I2C地址：0x3C（模块默认）
 *
 * 显示布局（128×64像素）：
 *   行1（顶部）：Target: XXX RPM
 *   行2         ：Actual: XXX RPM
 */

#include "i2c.h"
#include "stm32f10x.h"

void I2C_Init(void) {
    /* TODO: 步骤1 - 使能GPIOB、I2C1时钟 */

    /* TODO: 步骤2 - 配置PB6、PB7为复用开漏输出（I2C必须开漏） */

    /* TODO: 步骤3 - 配置I2C1
       I2C1->CR2   = 36;             // APB1频率36MHz
       I2C1->CCR   = 180;            // 标准模式100kHz：36MHz/(2×100kHz)=180
       I2C1->TRISE = 37;             // 最大上升时间
       I2C1->CR1  |= (1<<0);         // PE: 使能I2C
    */
}

/* 发送起始信号 */
static void I2C_Start(void) {
    /* TODO: I2C1->CR1 |= (1<<8); while(!(I2C1->SR1 & (1<<0))); */
}

/* 发送停止信号 */
static void I2C_Stop(void) {
    /* TODO: I2C1->CR1 |= (1<<9); */
}

/* 发送1字节并等待ACK */
static void I2C_SendByte(unsigned char byte) {
    /* TODO: I2C1->DR = byte; while(!(I2C1->SR1 & (1<<7))); // 等待TXE */
    (void)byte;
}

/* 向SSD1306发送命令字节 */
static void OLED_SendCmd(unsigned char cmd) {
    I2C_Start();
    I2C_SendByte(0x3C << 1); /* 设备地址 + 写位 */
    I2C_SendByte(0x00);      /* Control byte: Co=0, D/C=0（命令） */
    I2C_SendByte(cmd);
    I2C_Stop();
}

void OLED_Init(void) {
    /* SSD1306初始化序列（标准初始化命令，参考SSD1306数据手册） */
    OLED_SendCmd(0xAE); /* 关闭显示 */
    OLED_SendCmd(0x20); /* 内存地址模式 */
    OLED_SendCmd(0x00); /* 水平寻址 */
    OLED_SendCmd(0xB0); /* 页起始地址 */
    OLED_SendCmd(0xC8); /* COM输出方向 */
    OLED_SendCmd(0x00); /* 列低位地址 */
    OLED_SendCmd(0x10); /* 列高位地址 */
    OLED_SendCmd(0x40); /* 显示起始行 */
    OLED_SendCmd(0x81); /* 对比度控制 */
    OLED_SendCmd(0xFF); /* 对比度值（最亮） */
    OLED_SendCmd(0xA1); /* 段重映射 */
    OLED_SendCmd(0xA6); /* 正常显示（非反转） */
    OLED_SendCmd(0xA8); /* 多路复用比 */
    OLED_SendCmd(0x3F); /* 64行 */
    OLED_SendCmd(0xA4); /* 全部点亮模式关闭 */
    OLED_SendCmd(0xD3); /* 显示偏移 */
    OLED_SendCmd(0x00);
    OLED_SendCmd(0xD5); /* 时钟分频 */
    OLED_SendCmd(0xF0);
    OLED_SendCmd(0xD9); /* 预充电周期 */
    OLED_SendCmd(0x22);
    OLED_SendCmd(0xDA); /* COM引脚硬件配置 */
    OLED_SendCmd(0x12);
    OLED_SendCmd(0xDB); /* VCOMH去选择电平 */
    OLED_SendCmd(0x20);
    OLED_SendCmd(0x8D); /* 使能电荷泵 */
    OLED_SendCmd(0x14);
    OLED_SendCmd(0xAF); /* 打开显示 */
}

void OLED_Clear(void) {
    /* TODO: 向所有显示RAM写0x00清屏 */
}

void OLED_ShowSpeed(int target_rpm, int actual_rpm) {
    /*
     * TODO: 在OLED上显示两行文字：
     *   Line 1: "Target: XXX RPM"
     *   Line 2: "Actual: XXX RPM"
     *
     * 需要一套字库（5×7像素ASCII字库）
     * 可从网上找现成的SSD1306字库代码
     */
    (void)target_rpm;
    (void)actual_rpm;
}
