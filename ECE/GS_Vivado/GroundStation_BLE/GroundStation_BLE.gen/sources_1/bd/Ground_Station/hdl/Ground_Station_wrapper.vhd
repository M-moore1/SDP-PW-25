--Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
--Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
----------------------------------------------------------------------------------
--Tool Version: Vivado v.2025.1 (win64) Build 6140274 Thu May 22 00:12:29 MDT 2025
--Date        : Fri Feb 27 17:27:58 2026
--Host        : SDP2635 running 64-bit major release  (build 9200)
--Command     : generate_target Ground_Station_wrapper.bd
--Design      : Ground_Station_wrapper
--Purpose     : IP block netlist
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity Ground_Station_wrapper is
  port (
    BLE_RST : out STD_LOGIC_VECTOR ( 0 to 0 );
    BLE_RX : out STD_LOGIC;
    BLE_TX : in STD_LOGIC
  );
end Ground_Station_wrapper;

architecture STRUCTURE of Ground_Station_wrapper is
  component Ground_Station is
  port (
    BLE_TX : in STD_LOGIC;
    BLE_RX : out STD_LOGIC;
    BLE_RST : out STD_LOGIC_VECTOR ( 0 to 0 )
  );
  end component Ground_Station;
begin
Ground_Station_i: component Ground_Station
     port map (
      BLE_RST(0) => BLE_RST(0),
      BLE_RX => BLE_RX,
      BLE_TX => BLE_TX
    );
end STRUCTURE;
