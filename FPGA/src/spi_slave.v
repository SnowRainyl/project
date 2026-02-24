// spi_slave.v — SPI从机接收模块
//
// 功能：接收STM32发来的16位SPI数据，低10位作为PWM占空比输出
//
// SPI模式：CPOL=0, CPHA=0（时钟空闲低，上升沿采样）
// 数据格式：16位，高位先发（MSB first）
//
// 通信协议（与STM32 spi.c对应）：
//   SS拉低 → 接收16位数据 → SS拉高 → 更新占空比寄存器

module spi_slave (
    input  wire        clk,       // FPGA主时钟（用于同步）
    input  wire        rst_n,     // 复位（低有效）

    // SPI接口（来自STM32）
    input  wire        spi_sclk,  // SPI时钟
    input  wire        spi_mosi,  // 主机发送，从机接收
    input  wire        spi_ss,    // 片选（低有效）

    // 输出
    output reg  [9:0]  duty_out   // 解析出的占空比值（0~1000）
);

    // 接收移位寄存器（16位）
    reg [15:0] shift_reg;
    reg [3:0]  bit_cnt;     // 已接收的位数（0~15）

    // 检测spi_sclk上升沿（用主时钟同步）
    reg sclk_d1, sclk_d2;
    wire sclk_rising = sclk_d1 & ~sclk_d2;  // 上升沿

    // 检测ss下降沿（用于清零计数器）
    reg ss_d1, ss_d2;
    wire ss_falling = ~ss_d1 & ss_d2;       // 下降沿（SS从高到低）

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            sclk_d1  <= 0;
            sclk_d2  <= 0;
            ss_d1    <= 1;
            ss_d2    <= 1;
        end else begin
            // 同步采样
            sclk_d2 <= sclk_d1;
            sclk_d1 <= spi_sclk;
            ss_d2   <= ss_d1;
            ss_d1   <= spi_ss;
        end
    end

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            shift_reg <= 0;
            bit_cnt   <= 0;
            duty_out  <= 0;
        end else begin
            // SS下降沿：开始新帧，清零计数器
            if (ss_falling) begin
                bit_cnt <= 0;
            end

            // SPI时钟上升沿且SS有效（低电平）：采样MOSI
            if (sclk_rising && !spi_ss) begin
                shift_reg <= {shift_reg[14:0], spi_mosi}; // 高位先接收
                bit_cnt   <= bit_cnt + 1;

                // 接收满16位后更新输出
                if (bit_cnt == 15) begin
                    duty_out <= shift_reg[9:0]; // 取低10位作为占空比（0~1000）
                end
            end
        end
    end

endmodule
