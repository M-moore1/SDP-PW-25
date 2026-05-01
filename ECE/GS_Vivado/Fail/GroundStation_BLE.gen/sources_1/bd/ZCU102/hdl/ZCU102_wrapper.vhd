--Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
--Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
----------------------------------------------------------------------------------
--Tool Version: Vivado v.2025.1.1 (win64) Build 6233196 Thu Sep 11 21:27:30 MDT 2025
--Date        : Tue Dec  9 12:11:00 2025
--Host        : SDP2635 running 64-bit major release  (build 9200)
--Command     : generate_target ZCU102_wrapper.bd
--Design      : ZCU102_wrapper
--Purpose     : IP block netlist
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity ZCU102_wrapper is
  port (
    BT2_RX : out STD_LOGIC;
    BT2_TX : in STD_LOGIC
  );
end ZCU102_wrapper;

architecture STRUCTURE of ZCU102_wrapper is
  component ZCU102 is
  port (
    BT2_RX : out STD_LOGIC;
    BT2_TX : in STD_LOGIC
  );
  end component ZCU102;
begin
ZCU102_i: component ZCU102
     port map (
      BT2_RX => BT2_RX,
      BT2_TX => BT2_TX
    );
end STRUCTURE;
