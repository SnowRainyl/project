-- =============================================================================
-- spi_slave.vhd
--
-- 简化测试版：直接使用外部 SCK 上升沿接收 MOSI。
-- 注意：clk 端口保留用于兼容 top.vhd，本版本内部不使用 clk。
-- 帧格式：CS↓ → Byte0[3:0]=duty[11:8] → Byte1[7:0]=duty[7:0] → CS↑
-- =============================================================================

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity spi_slave is
    Port (
        clk        : in  STD_LOGIC;
        sck        : in  STD_LOGIC;
        cs_n       : in  STD_LOGIC;
        mosi       : in  STD_LOGIC;
        miso       : out STD_LOGIC;
        duty_out   : out STD_LOGIC_VECTOR(11 downto 0);
        duty_valid : out STD_LOGIC
    );
end spi_slave;

architecture Behavioral of spi_slave is

    -- -------------------------------------------------------------------------
    -- 接收逻辑
    -- -------------------------------------------------------------------------
    signal bit_cnt   : integer range 0 to 15 := 0;
    signal shift_reg : STD_LOGIC_VECTOR(7 downto 0) := (others => '0');
    signal temp_hi   : STD_LOGIC_VECTOR(3 downto 0) := (others => '0');
    signal valid_reg : STD_LOGIC := '0';

begin

    miso <= '0';
    duty_valid <= valid_reg;

    -- =========================================================================
    -- 接收状态机：CS_N 拉低后，每个 SCK 上升沿移入 1 bit
    -- =========================================================================
    p_recv : process(sck, cs_n)
    begin
        if cs_n = '1' then
            bit_cnt    <= 0;
            shift_reg  <= (others => '0');

        elsif rising_edge(sck) then
            valid_reg  <= '0';
            shift_reg  <= shift_reg(6 downto 0) & mosi;

            if bit_cnt = 7 then
                -- Byte0 收完，保存高 4 位
                temp_hi <= shift_reg(2 downto 0) & mosi;
                bit_cnt <= 8;

            elsif bit_cnt = 15 then
                -- Byte1 收完，组合 12bit 并输出
                duty_out <= temp_hi & (shift_reg(6 downto 0) & mosi);
                valid_reg <= '1';
                bit_cnt  <= 0;

            else
                bit_cnt <= bit_cnt + 1;
            end if;
        end if;
    end process;

end Behavioral;
