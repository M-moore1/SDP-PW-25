library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.gcm_pkg.all;
use work.aes_pkg.all;

entity tb_top_aes_gcm is
end entity;

architecture tb of tb_top_aes_gcm is

    ------------------------------------------------------------------------
    -- DUT Component Declaration
    ------------------------------------------------------------------------
    component top_aes_gcm is
        port(
            rst_i                       : in  std_logic;
            clk_i                       : in  std_logic;
            aes_gcm_mode_i              : in  std_logic_vector(1 downto 0);
            aes_gcm_enc_dec_i           : in  std_logic;
            aes_gcm_pipe_reset_i        : in  std_logic;
            aes_gcm_key_word_val_i      : in  std_logic_vector(3 downto 0);
            aes_gcm_key_word_i          : in  std_logic_vector(AES_256_KEY_WIDTH_C-1 downto 0);
            aes_gcm_iv_val_i            : in  std_logic;
            aes_gcm_iv_i                : in  std_logic_vector(GCM_ICB_WIDTH_C-1 downto 0);
            aes_gcm_icb_start_cnt_i     : in  std_logic;
            aes_gcm_icb_stop_cnt_i      : in  std_logic;
            aes_gcm_ghash_pkt_val_i     : in  std_logic;
            aes_gcm_ghash_aad_bval_i    : in  std_logic_vector(NB_STAGE_C-1 downto 0);
            aes_gcm_ghash_aad_i         : in  std_logic_vector(GCM_DATA_WIDTH_C-1 downto 0);
            aes_gcm_data_in_bval_i      : in  std_logic_vector(NB_STAGE_C-1 downto 0);
            aes_gcm_data_in_i           : in  std_logic_vector(AES_DATA_WIDTH_C-1 downto 0);
            aes_gcm_ready_o             : out std_logic;
            aes_gcm_data_out_val_o      : out std_logic;
            aes_gcm_data_out_bval_o     : out std_logic_vector(NB_STAGE_C-1 downto 0);
            aes_gcm_data_out_o          : out std_logic_vector(AES_DATA_WIDTH_C-1 downto 0);
            aes_gcm_ghash_tag_val_o     : out std_logic;
            aes_gcm_ghash_tag_o         : out std_logic_vector(GCM_DATA_WIDTH_C-1 downto 0);
            aes_gcm_icb_cnt_overflow_o  : out std_logic
        );
    end component;

    ------------------------------------------------------------------------
    -- Signals
    ------------------------------------------------------------------------
    signal clk                         : std_logic := '0';
    signal rst                         : std_logic := '1';

    signal aes_gcm_mode_i              : std_logic_vector(1 downto 0);
    signal aes_gcm_enc_dec_i           : std_logic;
    signal aes_gcm_pipe_reset_i        : std_logic;
    signal aes_gcm_key_word_val_i      : std_logic_vector(3 downto 0);
    signal aes_gcm_key_word_i          : std_logic_vector(AES_256_KEY_WIDTH_C-1 downto 0);
    signal aes_gcm_iv_val_i            : std_logic;
    signal aes_gcm_iv_i                : std_logic_vector(GCM_ICB_WIDTH_C-1 downto 0);
    signal aes_gcm_icb_start_cnt_i     : std_logic;
    signal aes_gcm_icb_stop_cnt_i      : std_logic;
    signal aes_gcm_ghash_pkt_val_i     : std_logic;
    signal aes_gcm_ghash_aad_bval_i    : std_logic_vector(NB_STAGE_C-1 downto 0);
    signal aes_gcm_ghash_aad_i         : std_logic_vector(GCM_DATA_WIDTH_C-1 downto 0);
    signal aes_gcm_data_in_bval_i      : std_logic_vector(NB_STAGE_C-1 downto 0);
    signal aes_gcm_data_in_i           : std_logic_vector(AES_DATA_WIDTH_C-1 downto 0);

    signal aes_gcm_ready_o             : std_logic;
    signal aes_gcm_data_out_val_o      : std_logic;
    signal aes_gcm_data_out_bval_o     : std_logic_vector(NB_STAGE_C-1 downto 0);
    signal aes_gcm_data_out_o          : std_logic_vector(AES_DATA_WIDTH_C-1 downto 0);
    signal aes_gcm_ghash_tag_val_o     : std_logic;
    signal aes_gcm_ghash_tag_o         : std_logic_vector(GCM_DATA_WIDTH_C-1 downto 0);
    signal aes_gcm_icb_cnt_overflow_o  : std_logic;

    ------------------------------------------------------------------------
    -- Clock Period
    ------------------------------------------------------------------------
    constant CLK_PERIOD : time := 10 ns;

begin

    ------------------------------------------------------------------------
    -- DUT Instantiation
    ------------------------------------------------------------------------
    DUT: top_aes_gcm
        port map(
            rst_i                       => rst,
            clk_i                       => clk,
            aes_gcm_mode_i              => aes_gcm_mode_i,
            aes_gcm_enc_dec_i           => aes_gcm_enc_dec_i,
            aes_gcm_pipe_reset_i        => aes_gcm_pipe_reset_i,
            aes_gcm_key_word_val_i      => aes_gcm_key_word_val_i,
            aes_gcm_key_word_i          => aes_gcm_key_word_i,
            aes_gcm_iv_val_i            => aes_gcm_iv_val_i,
            aes_gcm_iv_i                => aes_gcm_iv_i,
            aes_gcm_icb_start_cnt_i     => aes_gcm_icb_start_cnt_i,
            aes_gcm_icb_stop_cnt_i      => aes_gcm_icb_stop_cnt_i,
            aes_gcm_ghash_pkt_val_i     => aes_gcm_ghash_pkt_val_i,
            aes_gcm_ghash_aad_bval_i    => aes_gcm_ghash_aad_bval_i,
            aes_gcm_ghash_aad_i         => aes_gcm_ghash_aad_i,
            aes_gcm_data_in_bval_i      => aes_gcm_data_in_bval_i,
            aes_gcm_data_in_i           => aes_gcm_data_in_i,
            aes_gcm_ready_o             => aes_gcm_ready_o,
            aes_gcm_data_out_val_o      => aes_gcm_data_out_val_o,
            aes_gcm_data_out_bval_o     => aes_gcm_data_out_bval_o,
            aes_gcm_data_out_o          => aes_gcm_data_out_o,
            aes_gcm_ghash_tag_val_o     => aes_gcm_ghash_tag_val_o,
            aes_gcm_ghash_tag_o         => aes_gcm_ghash_tag_o,
            aes_gcm_icb_cnt_overflow_o  => aes_gcm_icb_cnt_overflow_o
        );

    ------------------------------------------------------------------------
    -- Clock Generation
    ------------------------------------------------------------------------
    clk_process : process
    begin
        while true loop
            clk <= '0';
            wait for CLK_PERIOD / 2;
            clk <= '1';
            wait for CLK_PERIOD / 2;
        end loop;
    end process;

    ------------------------------------------------------------------------
    -- Reset Process
    ------------------------------------------------------------------------
    rst_process : process
    begin
        rst <= '1';
        wait for 50 ns;
        rst <= '0';
        wait;
    end process;

    ------------------------------------------------------------------------
    -- Stimulus Process (YOU FILL THIS)
    ------------------------------------------------------------------------
    stim_proc : process
    begin
        --------------------------------------------------------------------
        -- Initialize inputs
        --------------------------------------------------------------------
        aes_gcm_mode_i           <= (others => '0');
        aes_gcm_enc_dec_i        <= '0';
        aes_gcm_pipe_reset_i     <= '0';
        aes_gcm_key_word_val_i   <= (others => '0');
        aes_gcm_key_word_i       <= (others => '0');
        aes_gcm_iv_val_i         <= '0';
        aes_gcm_iv_i             <= (others => '0');
        aes_gcm_icb_start_cnt_i  <= '0';
        aes_gcm_icb_stop_cnt_i   <= '0';
        aes_gcm_ghash_pkt_val_i  <= '0';
        aes_gcm_ghash_aad_bval_i <= (others => '0');
        aes_gcm_ghash_aad_i      <= (others => '0');
        aes_gcm_data_in_bval_i   <= (others => '0');
        aes_gcm_data_in_i        <= (others => '0');

        wait until rst = '0';
        wait for 20 ns;

        --------------------------------------------------------------------
        -- TODO: Insert your AES-GCM test sequence here
        -- 1. Load key
        -- 2. Load IV
        -- 3. Start counter
        -- 4. Feed AAD
        -- 5. Feed plaintext/ciphertext
        -- 6. Stop and check tag
        --------------------------------------------------------------------
        
        aes_gcm_enc_dec_i <= '1';   -- set to 1
        wait for 50 ns;             -- wait (adjust as needed)
        
        aes_gcm_enc_dec_i <= '0';   -- back to 0
        wait for 50 ns;

        wait;
    end process;

end architecture;