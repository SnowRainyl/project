-- spi_slave.vhd
-- SPI mode-0 slave. Samples MOSI on each SCK rising edge while CS_N is low.
-- Frame: CS_N low -> Byte0[3:0]=duty[11:8] -> Byte1[7:0]=duty[7:0] -> CS_N high.
-- clk port is kept for top-level wiring but is not used internally.

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

    signal bit_cnt   : integer range 0 to 15 := 0;
    signal shift_reg : STD_LOGIC_VECTOR(7 downto 0) := (others => '0');
    signal temp_hi   : STD_LOGIC_VECTOR(3 downto 0) := (others => '0');
    signal valid_reg : STD_LOGIC := '0';

begin

    miso <= '0';
    duty_valid <= valid_reg;

    p_recv : process(sck, cs_n)
    begin
        if cs_n = '1' then
            bit_cnt   <= 0;
            shift_reg <= (others => '0');
            valid_reg <= '0';

        elsif rising_edge(sck) then
            valid_reg <= '0';
            shift_reg <= shift_reg(6 downto 0) & mosi;

            if bit_cnt = 7 then
                temp_hi <= shift_reg(2 downto 0) & mosi;  -- save high nibble after byte 0
                bit_cnt <= 8;

            elsif bit_cnt = 15 then
                duty_out  <= temp_hi & (shift_reg(6 downto 0) & mosi);
                valid_reg <= '1';
                bit_cnt   <= 0;

            else
                bit_cnt <= bit_cnt + 1;
            end if;
        end if;
    end process;

end Behavioral;
