-- pwm_gen.vhd
-- 12-bit PWM generator. Frequency: 50MHz / 4096 = 12.2kHz. duty=0->0%, duty=4095->99.98%

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity pwm_gen is
    Port (
        clk        : in  STD_LOGIC;
        duty       : in  STD_LOGIC_VECTOR(11 downto 0);
        duty_valid : in  STD_LOGIC;
        pwm_out    : out STD_LOGIC
    );
end pwm_gen;

architecture Behavioral of pwm_gen is

    signal counter      : unsigned(11 downto 0) := (others => '0');
    signal duty_latched : unsigned(11 downto 0) := (others => '0');

begin

    p_latch : process(clk)
    begin
        if rising_edge(clk) then
            if duty_valid = '1' then
                duty_latched <= unsigned(duty);
            end if;
        end if;
    end process;

    p_counter : process(clk)
    begin
        if rising_edge(clk) then
            counter <= counter + 1;
        end if;
    end process;

    pwm_out <= '1' when counter < duty_latched else '0';

end Behavioral;
