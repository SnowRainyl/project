// top.v — 顶层模块
//
// 连接 spi_slave 和 pwm_gen 两个子模块
// 这个文件是综合的入口，在ISE里设置为顶层模块

module top (
    input  wire clk,        // FPGA主时钟（AX309板载 50MHz）
    input  wire rst_n,      // 复位按钮（低有效）

    // SPI接口（连接STM32）
    input  wire spi_sclk,
    input  wire spi_mosi,
    input  wire spi_ss,

    // PWM输出（连接TB6612电机驱动的PWMA引脚）
    output wire pwm_out
);

    // spi_slave → pwm_gen 之间的内部连线
    wire [9:0] duty;  // 占空比值（0~1000）

    // SPI从机：接收STM32发来的占空比指令
    spi_slave u_spi_slave (
        .clk      (clk),
        .rst_n    (rst_n),
        .spi_sclk (spi_sclk),
        .spi_mosi (spi_mosi),
        .spi_ss   (spi_ss),
        .duty_out (duty)
    );

    // PWM生成器：根据占空比输出PWM波
    pwm_gen u_pwm_gen (
        .clk     (clk),
        .rst_n   (rst_n),
        .duty    (duty),
        .pwm_out (pwm_out)
    );

endmodule
