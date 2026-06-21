-- =============================================================================
-- spi_slave.vhd
--
-- 所有逻辑在 CLK 域内运行，用 2 级同步链消除 SCK/CS_N 的跨域问题。
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
    -- 2 级同步链：消除亚稳态，所有信号同步到 CLK 域
    -- -------------------------------------------------------------------------
    signal sck_d1  : STD_LOGIC := '0';
    signal sck_d2  : STD_LOGIC := '0';
    signal cs_d1   : STD_LOGIC := '1';
    signal cs_d2   : STD_LOGIC := '1';
    signal mosi_d1 : STD_LOGIC := '0';

    signal sck_rise : STD_LOGIC;   -- SCK 上升沿脉冲（单 CLK 周期）
    signal cs_rise  : STD_LOGIC;   -- CS_N 上升沿脉冲（帧结束）

    -- -------------------------------------------------------------------------
    -- 接收逻辑
    -- -------------------------------------------------------------------------
    signal bit_cnt   : integer range 0 to 15 := 0;
    signal shift_reg : STD_LOGIC_VECTOR(7 downto 0) := (others => '0');
    signal temp_hi   : STD_LOGIC_VECTOR(3 downto 0) := (others => '0');

begin

    miso <= '0';

    -- =========================================================================
    -- 同步链：每个 CLK 打两拍
    -- =========================================================================
    p_sync : process(clk)
    begin
        if rising_edge(clk) then
            sck_d1  <= sck;
            sck_d2  <= sck_d1;
            cs_d1   <= cs_n;
            cs_d2   <= cs_d1;
            mosi_d1 <= mosi;
        end if;
    end process;

    -- sck_d1='1' 且 sck_d2='0'：SCK 上升沿（已稳定）
    sck_rise <= sck_d1 and (not sck_d2);
    -- cs_d1='1' 且 cs_d2='0'：CS_N 上升沿（帧结束）
    cs_rise  <= cs_d1  and (not cs_d2);

    -- =========================================================================
    -- 接收状态机：全部在 CLK 域，用 sck_rise / cs_rise 脉冲驱动
    -- =========================================================================
    p_recv : process(clk)
    begin
        if rising_edge(clk) then
            duty_valid <= '0';

            -- CS 上升沿：锁存本帧结果，重置计数器
            if cs_rise = '1' then
                duty_valid <= '1';
                bit_cnt    <= 0;
                shift_reg  <= (others => '0');

            -- SCK 上升沿：移入数据
            elsif sck_rise = '1' and cs_d1 = '0' then
                shift_reg <= shift_reg(6 downto 0) & mosi_d1;

                if bit_cnt = 7 then
                    -- Byte0 收完，保存高 4 位
                    temp_hi <= shift_reg(2 downto 0) & mosi_d1;
                    bit_cnt <= 8;

                elsif bit_cnt = 15 then
                    -- Byte1 收完，组合 12bit 并输出
                    duty_out <= temp_hi & (shift_reg(6 downto 0) & mosi_d1);
                    bit_cnt  <= 0;

                else
                    bit_cnt <= bit_cnt + 1;
                end if;
            end if;
        end if;
    end process;

end Behavioral;
