--------------------------------------------------------------------------------
--! @File name:     aes_kexp
--! @Date:          12/02/2016
--! @Description:   the module performs the key expansion
--! @Reference:     FIPS PUB 197, November 26, 2001
--! @Source:        http://csrc.nist.gov/publications/fips/fips197/fips-197.pdf
--------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use work.aes_pkg.all;
use work.aes_func.all;

--------------------------------------------------------------------------------
entity aes_kexp is
    generic(
        core_num_g              : natural := 0);
    port(
        rst_i                   : in  std_logic;
        clk_i                   : in  std_logic;
        aes_mode_i              : in  std_logic_vector(1 downto 0);
        kexp_cnt_i              : in  round_cnt_t;
        kexp_dval_i             : in  std_logic;
        kexp_rcon_i             : in  byte_t;
        kexp_key_part_i         : in  key_vec_t;
        kexp_rcon_o             : out byte_t;
        kexp_key_next_part_o    : out key_vec_t;
        kexp_key_next_stage_o   : out state_t;
        kexp_key_last_stage_o   : out state_t);
end entity;

--------------------------------------------------------------------------------
architecture arch_aes_kexp of aes_kexp is

    --! Constants
    constant RST_WORD_C          : word_t := (x"00", x"00", x"00", x"00");

    --! Types

    --! Signals

    signal w_in_0                : word_t;
    signal w_in_1                : word_t;
    signal w_in_2                : word_t;
    signal w_in_3                : word_t;

    signal w_in_4                : word_t;
    signal w_in_5                : word_t;
    signal w_in_6                : word_t;
    signal w_in_7                : word_t;

    signal w_0_q                 : word_t;
    signal w_1_q                 : word_t;
    signal w_2_q                 : word_t;
    signal w_3_q                 : word_t;
    signal w_4_q                 : word_t;
    signal w_5_q                 : word_t;
    signal w_6_q                 : word_t;
    signal w_7_q                 : word_t;

    signal w_0                   : word_t;
    signal w_1                   : word_t;
    signal w_2                   : word_t;
    signal w_3                   : word_t;

    signal opa_0                 : word_t;
    signal opa_1                 : word_t;
    signal opa_2                 : word_t;
    signal opa_3                 : word_t;

    signal opb_0                 : word_t;
    signal opb_1                 : word_t;
    signal opb_2                 : word_t;
    signal opb_3                 : word_t;

    signal rcon_next             : byte_t;
    signal rcon_byte_c           : byte_t;
    signal kexp_rcon_q           : byte_t;
    signal rcon_c                : word_t;

    signal tmp                   : word_t;
    signal rotw                  : word_t;
    signal subw                  : word_t;
    signal elabw                 : word_t;

    signal skip_192              : std_logic;
    signal skip_256              : std_logic;

    signal kexp_key_next_part    : key_vec_t;
    signal kexp_key_next_stage   : state_t;
    signal kexp_key_last_stage   : state_t;

    signal kexp_var_en           : std_logic_vector(2 downto 0);

begin

    w_in_7      <= kexp_key_part_i(7);
    w_in_6      <= kexp_key_part_i(6);
    w_in_5      <= kexp_key_part_i(5);
    w_in_4      <= kexp_key_part_i(4);
    w_in_3      <= kexp_key_part_i(3);
    w_in_2      <= kexp_key_part_i(2);
    w_in_1      <= kexp_key_part_i(1);
    w_in_0      <= kexp_key_part_i(0);

    opb_0       <= w_in_7;
    opb_1       <= w_in_6;
    opb_2       <= w_in_5;
    opb_3       <= w_in_4;

    --! Word to be expanded
    tmp         <=  w_in_4      when ( aes_mode_i = AES_MODE_128_C) else
                    w_in_0      when ( aes_mode_i = AES_MODE_256_C) else
                    w_in_2      when ((aes_mode_i = AES_MODE_192_C) and (kexp_var_en(0) = '1')) else
                    w_xor(w_xor(w_in_6, w_in_7), w_in_2);

    opa_0       <=  w_in_2    when ((aes_mode_i = AES_MODE_192_C) and (kexp_var_en(0) = '0')) else elabw;

    opa_2       <=  elabw       when ((aes_mode_i = AES_MODE_192_C) and (kexp_var_en(1) = '1')) else w_1;

    opa_1       <=  w_0;
    opa_3       <=  w_2;


    --! Shift, Rotate, Substitute and xor operations
    skip_192    <=  '1' when ((aes_mode_i = AES_MODE_192_C) and (kexp_var_en(1 downto 0) = "00"))   else '0';
    skip_256    <=  '1' when ((aes_mode_i = AES_MODE_256_C) and (kexp_var_en(2) = '1')) else '0';

    --! introduce skip_192 and skip_256

    rcon_byte_c <=  kexp_rcon_i when (skip_256 = '0') else x"00";
    rcon_c      <=  (rcon_byte_c, x"00", x"00", x"00");

    rcon_next   <=  kexp_rcon_i             when (skip_256 = '1' or skip_192 = '1') else    xtime2(kexp_rcon_i);
    rotw        <=  tmp                     when (skip_256 = '1')                     else    rot_word(tmp);
    subw        <=  sub_word(rotw);
    elabw       <=  w_xor(rcon_c, subw);

    --! Execute Xor between expanded and incoming key
    w_0         <= w_xor(opa_0, opb_0);
    w_1         <= w_xor(opa_1, opb_1);
    w_2         <= w_xor(opa_2, opb_2);
    w_3         <= w_xor(opa_3, opb_3);

    --------------------------------------------------------------------------------
    --! process: Sample new rcon
    --------------------------------------------------------------------------------
    new_rcon_p: process(rst_i, clk_i)
    begin
        if(rst_i = '1') then
            kexp_rcon_q <= (others => '0');
        elsif(rising_edge(clk_i)) then
            if(kexp_dval_i = '1') then
                kexp_rcon_q <= rcon_next;
            end if;
        end if;
    end process;

    --------------------------------------------------------------------------------
    --! process: Sample key
    --------------------------------------------------------------------------------
    sample_key_p: process(rst_i, clk_i)
    begin
        if(rst_i = '1') then
            w_0_q   <= RST_WORD_C;
            w_1_q   <= RST_WORD_C;
            w_2_q   <= RST_WORD_C;
            w_3_q   <= RST_WORD_C;
            w_4_q   <= RST_WORD_C;
            w_5_q   <= RST_WORD_C;
            w_6_q   <= RST_WORD_C;
            w_7_q   <= RST_WORD_C;
        elsif(rising_edge(clk_i)) then
            if(kexp_dval_i = '1') then
                if(aes_mode_i = AES_MODE_128_C) then
                    w_7_q <= w_0;
                    w_6_q <= w_1;
                    w_5_q <= w_2;
                    w_4_q <= w_3;
                    w_3_q <= RST_WORD_C;
                    w_2_q <= RST_WORD_C;
                    w_1_q <= RST_WORD_C;
                    w_0_q <= RST_WORD_C;
                elsif(aes_mode_i = AES_MODE_192_C) then
                    w_7_q <= w_in_3;
                    w_6_q <= w_in_2;
                    w_5_q <= w_0;
                    w_4_q <= w_1;
                    w_3_q <= w_2;
                    w_2_q <= w_3;
                    w_1_q <= RST_WORD_C;
                    w_0_q <= RST_WORD_C;
                else
                    w_7_q <= w_in_3;
                    w_6_q <= w_in_2;
                    w_5_q <= w_in_1;
                    w_4_q <= w_in_0;
                    w_3_q <= w_0;
                    w_2_q <= w_1;
                    w_1_q <= w_2;
                    w_0_q <= w_3;
                end if;
            end if;
        end if;
    end process;

    kexp_key_next_part    <= (w_7_q, w_6_q, w_5_q, w_4_q, w_3_q, w_2_q, w_1_q, w_0_q);
    kexp_key_next_stage   <= (kexp_key_part_i(7), kexp_key_part_i(6), kexp_key_part_i(5), kexp_key_part_i(4));
    kexp_key_last_stage   <= (w_7_q, w_6_q, w_5_q, w_4_q);

    
    gen_key_var_0: if core_num_g = 0 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
        end process;
    end generate;

    gen_key_var_1: if core_num_g = 1 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
            case kexp_cnt_i is
                when 2 => kexp_var_en(2) <= '1';
                when others => kexp_var_en(2) <= '0';
            end case;
        end process;
    end generate;

    gen_key_var_2: if core_num_g = 2 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
        end process;
    end generate;

    gen_key_var_3: if core_num_g = 3 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
            case kexp_cnt_i is
                when 4 => kexp_var_en(2) <= '1';
                when others => kexp_var_en(2) <= '0';
            end case;
        end process;
    end generate;

    gen_key_var_4: if core_num_g = 4 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
        end process;
    end generate;

    gen_key_var_5: if core_num_g = 5 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
            case kexp_cnt_i is
                when 6 => kexp_var_en(2) <= '1';
                when others => kexp_var_en(2) <= '0';
            end case;
        end process;
    end generate;

    gen_key_var_6: if core_num_g = 6 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
        end process;
    end generate;

    gen_key_var_7: if core_num_g = 7 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
            case kexp_cnt_i is
                when 8 => kexp_var_en(2) <= '1';
                when others => kexp_var_en(2) <= '0';
            end case;
        end process;
    end generate;

    gen_key_var_8: if core_num_g = 8 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
        end process;
    end generate;

    gen_key_var_9: if core_num_g = 9 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
            case kexp_cnt_i is
                when 10 => kexp_var_en(2) <= '1';
                when others => kexp_var_en(2) <= '0';
            end case;
        end process;
    end generate;

    gen_key_var_10: if core_num_g = 10 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
        end process;
    end generate;

    gen_key_var_11: if core_num_g = 11 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
            case kexp_cnt_i is
                when 12 => kexp_var_en(2) <= '1';
                when others => kexp_var_en(2) <= '0';
            end case;
        end process;
    end generate;

    gen_key_var_12: if core_num_g = 12 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
        end process;
    end generate;

    gen_key_var_13: if core_num_g = 13 generate
        process(kexp_cnt_i)
        begin
            kexp_var_en <= "000";
            case kexp_cnt_i is
                when 14 => kexp_var_en(2) <= '1';
                when others => kexp_var_en(2) <= '0';
            end case;
        end process;
    end generate;


    --! Outpus
    kexp_key_next_stage_o   <= kexp_key_next_stage;
    kexp_key_last_stage_o   <= kexp_key_last_stage;
    kexp_rcon_o             <= kexp_rcon_q;
    kexp_key_next_part_o    <= kexp_key_next_part;

end architecture;
