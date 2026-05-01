library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity simple_adder is
    Port (
        clk        : in  std_logic;
        rst        : in  std_logic;
        enable     : in  std_logic;
        input_val  : in  std_logic_vector(31 downto 0);
        output_val : out std_logic_vector(31 downto 0)
    );
end simple_adder;

architecture Behavioral of simple_adder is
    signal result_reg : std_logic_vector(31 downto 0);
begin

    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                result_reg <= (others => '0');

            elsif enable = '1' then
                -- Only update when enabled
                result_reg <= std_logic_vector(unsigned(input_val) + 2);

            else
                -- Hold value when disabled
                result_reg <= result_reg;
            end if;
        end if;
    end process;

    output_val <= result_reg;

end Behavioral;