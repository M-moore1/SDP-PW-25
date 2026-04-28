--Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
--Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
----------------------------------------------------------------------------------
--Tool Version: Vivado v.2025.1 (win64) Build 6140274 Thu May 22 00:12:29 MDT 2025
--Date        : Sat Apr  4 22:03:42 2026
--Host        : SDP2635 running 64-bit major release  (build 9200)
--Command     : generate_target design_1_wrapper.bd
--Design      : design_1_wrapper
--Purpose     : IP block netlist
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity design_1_wrapper is
  port (
    BLE_CTS : out STD_LOGIC;
    BLE_PINS_tri_io : inout STD_LOGIC_VECTOR ( 3 downto 0 );
    BLE_RTS : in STD_LOGIC;
    BLE_RX : out STD_LOGIC;
    BLE_TX : in STD_LOGIC;
    ESP_PINS_tri_io : inout STD_LOGIC_VECTOR ( 3 downto 0 );
    ESP_RX : out STD_LOGIC;
    ESP_TX : in STD_LOGIC
  );
end design_1_wrapper;

architecture STRUCTURE of design_1_wrapper is
  component design_1 is
  port (
    BLE_PINS_tri_i : in STD_LOGIC_VECTOR ( 3 downto 0 );
    BLE_PINS_tri_o : out STD_LOGIC_VECTOR ( 3 downto 0 );
    BLE_PINS_tri_t : out STD_LOGIC_VECTOR ( 3 downto 0 );
    ESP_PINS_tri_i : in STD_LOGIC_VECTOR ( 3 downto 0 );
    ESP_PINS_tri_o : out STD_LOGIC_VECTOR ( 3 downto 0 );
    ESP_PINS_tri_t : out STD_LOGIC_VECTOR ( 3 downto 0 );
    BLE_RX : out STD_LOGIC;
    BLE_TX : in STD_LOGIC;
    BLE_RTS : in STD_LOGIC;
    BLE_CTS : out STD_LOGIC;
    ESP_TX : in STD_LOGIC;
    ESP_RX : out STD_LOGIC
  );
  end component design_1;
  component IOBUF is
  port (
    I : in STD_LOGIC;
    O : out STD_LOGIC;
    T : in STD_LOGIC;
    IO : inout STD_LOGIC
  );
  end component IOBUF;
  signal BLE_PINS_tri_i_0 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal BLE_PINS_tri_i_1 : STD_LOGIC_VECTOR ( 1 to 1 );
  signal BLE_PINS_tri_i_2 : STD_LOGIC_VECTOR ( 2 to 2 );
  signal BLE_PINS_tri_i_3 : STD_LOGIC_VECTOR ( 3 to 3 );
  signal BLE_PINS_tri_io_0 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal BLE_PINS_tri_io_1 : STD_LOGIC_VECTOR ( 1 to 1 );
  signal BLE_PINS_tri_io_2 : STD_LOGIC_VECTOR ( 2 to 2 );
  signal BLE_PINS_tri_io_3 : STD_LOGIC_VECTOR ( 3 to 3 );
  signal BLE_PINS_tri_o_0 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal BLE_PINS_tri_o_1 : STD_LOGIC_VECTOR ( 1 to 1 );
  signal BLE_PINS_tri_o_2 : STD_LOGIC_VECTOR ( 2 to 2 );
  signal BLE_PINS_tri_o_3 : STD_LOGIC_VECTOR ( 3 to 3 );
  signal BLE_PINS_tri_t_0 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal BLE_PINS_tri_t_1 : STD_LOGIC_VECTOR ( 1 to 1 );
  signal BLE_PINS_tri_t_2 : STD_LOGIC_VECTOR ( 2 to 2 );
  signal BLE_PINS_tri_t_3 : STD_LOGIC_VECTOR ( 3 to 3 );
  signal ESP_PINS_tri_i_0 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal ESP_PINS_tri_i_1 : STD_LOGIC_VECTOR ( 1 to 1 );
  signal ESP_PINS_tri_i_2 : STD_LOGIC_VECTOR ( 2 to 2 );
  signal ESP_PINS_tri_i_3 : STD_LOGIC_VECTOR ( 3 to 3 );
  signal ESP_PINS_tri_io_0 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal ESP_PINS_tri_io_1 : STD_LOGIC_VECTOR ( 1 to 1 );
  signal ESP_PINS_tri_io_2 : STD_LOGIC_VECTOR ( 2 to 2 );
  signal ESP_PINS_tri_io_3 : STD_LOGIC_VECTOR ( 3 to 3 );
  signal ESP_PINS_tri_o_0 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal ESP_PINS_tri_o_1 : STD_LOGIC_VECTOR ( 1 to 1 );
  signal ESP_PINS_tri_o_2 : STD_LOGIC_VECTOR ( 2 to 2 );
  signal ESP_PINS_tri_o_3 : STD_LOGIC_VECTOR ( 3 to 3 );
  signal ESP_PINS_tri_t_0 : STD_LOGIC_VECTOR ( 0 to 0 );
  signal ESP_PINS_tri_t_1 : STD_LOGIC_VECTOR ( 1 to 1 );
  signal ESP_PINS_tri_t_2 : STD_LOGIC_VECTOR ( 2 to 2 );
  signal ESP_PINS_tri_t_3 : STD_LOGIC_VECTOR ( 3 to 3 );
begin
BLE_PINS_tri_iobuf_0: component IOBUF
     port map (
      I => BLE_PINS_tri_o_0(0),
      IO => BLE_PINS_tri_io(0),
      O => BLE_PINS_tri_i_0(0),
      T => BLE_PINS_tri_t_0(0)
    );
BLE_PINS_tri_iobuf_1: component IOBUF
     port map (
      I => BLE_PINS_tri_o_1(1),
      IO => BLE_PINS_tri_io(1),
      O => BLE_PINS_tri_i_1(1),
      T => BLE_PINS_tri_t_1(1)
    );
BLE_PINS_tri_iobuf_2: component IOBUF
     port map (
      I => BLE_PINS_tri_o_2(2),
      IO => BLE_PINS_tri_io(2),
      O => BLE_PINS_tri_i_2(2),
      T => BLE_PINS_tri_t_2(2)
    );
BLE_PINS_tri_iobuf_3: component IOBUF
     port map (
      I => BLE_PINS_tri_o_3(3),
      IO => BLE_PINS_tri_io(3),
      O => BLE_PINS_tri_i_3(3),
      T => BLE_PINS_tri_t_3(3)
    );
ESP_PINS_tri_iobuf_0: component IOBUF
     port map (
      I => ESP_PINS_tri_o_0(0),
      IO => ESP_PINS_tri_io(0),
      O => ESP_PINS_tri_i_0(0),
      T => ESP_PINS_tri_t_0(0)
    );
ESP_PINS_tri_iobuf_1: component IOBUF
     port map (
      I => ESP_PINS_tri_o_1(1),
      IO => ESP_PINS_tri_io(1),
      O => ESP_PINS_tri_i_1(1),
      T => ESP_PINS_tri_t_1(1)
    );
ESP_PINS_tri_iobuf_2: component IOBUF
     port map (
      I => ESP_PINS_tri_o_2(2),
      IO => ESP_PINS_tri_io(2),
      O => ESP_PINS_tri_i_2(2),
      T => ESP_PINS_tri_t_2(2)
    );
ESP_PINS_tri_iobuf_3: component IOBUF
     port map (
      I => ESP_PINS_tri_o_3(3),
      IO => ESP_PINS_tri_io(3),
      O => ESP_PINS_tri_i_3(3),
      T => ESP_PINS_tri_t_3(3)
    );
design_1_i: component design_1
     port map (
      BLE_CTS => BLE_CTS,
      BLE_PINS_tri_i(3) => BLE_PINS_tri_i_3(3),
      BLE_PINS_tri_i(2) => BLE_PINS_tri_i_2(2),
      BLE_PINS_tri_i(1) => BLE_PINS_tri_i_1(1),
      BLE_PINS_tri_i(0) => BLE_PINS_tri_i_0(0),
      BLE_PINS_tri_o(3) => BLE_PINS_tri_o_3(3),
      BLE_PINS_tri_o(2) => BLE_PINS_tri_o_2(2),
      BLE_PINS_tri_o(1) => BLE_PINS_tri_o_1(1),
      BLE_PINS_tri_o(0) => BLE_PINS_tri_o_0(0),
      BLE_PINS_tri_t(3) => BLE_PINS_tri_t_3(3),
      BLE_PINS_tri_t(2) => BLE_PINS_tri_t_2(2),
      BLE_PINS_tri_t(1) => BLE_PINS_tri_t_1(1),
      BLE_PINS_tri_t(0) => BLE_PINS_tri_t_0(0),
      BLE_RTS => BLE_RTS,
      BLE_RX => BLE_RX,
      BLE_TX => BLE_TX,
      ESP_PINS_tri_i(3) => ESP_PINS_tri_i_3(3),
      ESP_PINS_tri_i(2) => ESP_PINS_tri_i_2(2),
      ESP_PINS_tri_i(1) => ESP_PINS_tri_i_1(1),
      ESP_PINS_tri_i(0) => ESP_PINS_tri_i_0(0),
      ESP_PINS_tri_o(3) => ESP_PINS_tri_o_3(3),
      ESP_PINS_tri_o(2) => ESP_PINS_tri_o_2(2),
      ESP_PINS_tri_o(1) => ESP_PINS_tri_o_1(1),
      ESP_PINS_tri_o(0) => ESP_PINS_tri_o_0(0),
      ESP_PINS_tri_t(3) => ESP_PINS_tri_t_3(3),
      ESP_PINS_tri_t(2) => ESP_PINS_tri_t_2(2),
      ESP_PINS_tri_t(1) => ESP_PINS_tri_t_1(1),
      ESP_PINS_tri_t(0) => ESP_PINS_tri_t_0(0),
      ESP_RX => ESP_RX,
      ESP_TX => ESP_TX
    );
end STRUCTURE;
