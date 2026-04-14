-- =============================================================================
-- top.vhd  （无复位版，上电直接运行）
--
-- 引脚：
--   clk     : FPGA 系统时钟（50MHz）
--   sck     : STM32 PB13（SPI2_SCK）
--   cs_n    : STM32 PB12（SPI2_CS）
--   mosi    : STM32 PB15（SPI2_MOSI）
--   miso    : STM32 PB14（SPI2_MISO，未使用）
--   pwm_out : 电机驱动器 PWM 输入
-- =============================================================================

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity top is
    Port (
        clk     : in  STD_LOGIC;
        sck     : in  STD_LOGIC;
        cs_n    : in  STD_LOGIC;
        mosi    : in  STD_LOGIC;
        miso    : out STD_LOGIC;
        pwm_out : out STD_LOGIC
    );
end top;

architecture Structural of top is

    component spi_slave is
        Port (
            clk        : in  STD_LOGIC;
            sck        : in  STD_LOGIC;
            cs_n       : in  STD_LOGIC;
            mosi       : in  STD_LOGIC;
            miso       : out STD_LOGIC;
            duty_out   : out STD_LOGIC_VECTOR(11 downto 0);
            duty_valid : out STD_LOGIC
        );
    end component;

    component pwm_gen is
        Port (
            clk        : in  STD_LOGIC;
            duty       : in  STD_LOGIC_VECTOR(11 downto 0);
            duty_valid : in  STD_LOGIC;
            pwm_out    : out STD_LOGIC
        );
    end component;

    signal duty_wire  : STD_LOGIC_VECTOR(11 downto 0);
    signal valid_wire : STD_LOGIC;

begin

    u_spi_slave : spi_slave
        port map (
            clk        => clk,
            sck        => sck,
            cs_n       => cs_n,
            mosi       => mosi,
            miso       => miso,
            duty_out   => duty_wire,
            duty_valid => valid_wire
        );

    u_pwm_gen : pwm_gen
        port map (
            clk        => clk,
            duty       => duty_wire,
            duty_valid => valid_wire,
            pwm_out    => pwm_out
        );

end Structural;
