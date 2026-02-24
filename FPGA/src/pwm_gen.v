// pwm_gen.v — PWM波形生成模块
//
// 原理：用计数器计数，当计数值 < 占空比设定值时输出高电平
//
// 参数：
//   CLK_FREQ  = 50_000_000  (FPGA主时钟 50MHz)
//   PWM_FREQ  = 20_000      (PWM频率 20kHz，适合电机驱动)
//   DUTY_MAX  = 1000        (占空比分辨率：0~1000 对应 0%~100%)
//
// 例：占空比 = 500，则 PWM = 50%，电机以50%功率运转

module pwm_gen (
    input  wire        clk,       // FPGA主时钟（50MHz）
    input  wire        rst_n,     // 复位（低有效）
    input  wire [9:0]  duty,      // 占空比设定值（0~1000）
    output reg         pwm_out    // PWM输出
);

    // 每个PWM周期需要的时钟数
    // PWM周期 = CLK_FREQ / PWM_FREQ = 50MHz / 20kHz = 2500 个时钟
    parameter CLK_FREQ = 50_000_000;
    parameter PWM_FREQ = 20_000;
    parameter PERIOD   = CLK_FREQ / PWM_FREQ; // = 2500

    // 计数器（计数范围 0 ~ PERIOD-1）
    reg [11:0] counter; // 12位足够计到2500

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            counter <= 0;
            pwm_out <= 0;
        end else begin
            // 计数器循环
            if (counter >= PERIOD - 1)
                counter <= 0;
            else
                counter <= counter + 1;

            // 比较：计数值 × 1000 < duty × PERIOD
            // 等价于：counter < duty * PERIOD / 1000
            // 用移位避免除法：这里直接用乘法比较
            if (counter * 1000 < duty * PERIOD)
                pwm_out <= 1;
            else
                pwm_out <= 0;
        end
    end

endmodule
