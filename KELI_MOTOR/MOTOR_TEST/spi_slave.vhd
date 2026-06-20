-- spi_slave.vhd
-- All logic runs in the CLK domain; 2-stage sync chain resolves SCK/CS_N clock crossing.
-- Frame format: CS low -> Byte0[3:0]=duty[11:8] -> Byte1[7:0]=duty[7:0] -> CS high

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

    -- 2-stage synchroniser: removes metastability on SCK and CS_N
    signal sck_d1  : STD_LOGIC := '0';
    signal sck_d2  : STD_LOGIC := '0';
    signal cs_d1   : STD_LOGIC := '1';
    signal cs_d2   : STD_LOGIC := '1';
    signal mosi_d1 : STD_LOGIC := '0';

    signal sck_rise : STD_LOGIC;  -- one-CLK pulse on SCK rising edge
    signal cs_rise  : STD_LOGIC;  -- one-CLK pulse on CS_N rising edge (end of frame)

    signal bit_cnt   : integer range 0 to 15 := 0;
    signal shift_reg : STD_LOGIC_VECTOR(7 downto 0) := (others => '0');
    signal temp_hi   : STD_LOGIC_VECTOR(3 downto 0) := (others => '0');

begin

    miso <= '0';

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

    sck_rise <= sck_d1 and (not sck_d2);
    cs_rise  <= cs_d1  and (not cs_d2);

    p_recv : process(clk)
    begin
        if rising_edge(clk) then
            duty_valid <= '0';

            if cs_rise = '1' then
                -- CS rising edge: latch result and reset for next frame
                duty_valid <= '1';
                bit_cnt    <= 0;
                shift_reg  <= (others => '0');

            elsif sck_rise = '1' and cs_d1 = '0' then
                shift_reg <= shift_reg(6 downto 0) & mosi_d1;

                if bit_cnt = 7 then
                    -- Byte0 done: save upper 4 bits of the 12-bit duty
                    temp_hi <= shift_reg(2 downto 0) & mosi_d1;
                    bit_cnt <= 8;

                elsif bit_cnt = 15 then
                    -- Byte1 done: assemble full 12-bit duty value
                    duty_out <= temp_hi & (shift_reg(6 downto 0) & mosi_d1);
                    bit_cnt  <= 0;

                else
                    bit_cnt <= bit_cnt + 1;
                end if;
            end if;
        end if;
    end process;

end Behavioral;
