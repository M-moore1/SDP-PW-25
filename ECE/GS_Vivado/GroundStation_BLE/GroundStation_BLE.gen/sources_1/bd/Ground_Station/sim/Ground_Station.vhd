--Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
--Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
----------------------------------------------------------------------------------
--Tool Version: Vivado v.2025.1 (win64) Build 6140274 Thu May 22 00:12:29 MDT 2025
--Date        : Fri Feb 27 17:27:58 2026
--Host        : SDP2635 running 64-bit major release  (build 9200)
--Command     : generate_target Ground_Station.bd
--Design      : Ground_Station
--Purpose     : IP block netlist
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity Ground_Station is
  port (
    BLE_RST : out STD_LOGIC_VECTOR ( 0 to 0 );
    BLE_RX : out STD_LOGIC;
    BLE_TX : in STD_LOGIC
  );
  attribute CORE_GENERATION_INFO : string;
  attribute CORE_GENERATION_INFO of Ground_Station : entity is "Ground_Station,IP_Integrator,{x_ipVendor=xilinx.com,x_ipLibrary=BlockDiagram,x_ipName=Ground_Station,x_ipVersion=1.00.a,x_ipLanguage=VHDL,numBlks=2,numReposBlks=2,numNonXlnxBlks=0,numHierBlks=0,maxHierDepth=0,numSysgenBlks=0,numHlsBlks=0,numHdlrefBlks=0,numPkgbdBlks=0,bdsource=USER,da_zynq_ultra_ps_e_cnt=1,synth_mode=Hierarchical}";
  attribute HW_HANDOFF : string;
  attribute HW_HANDOFF of Ground_Station : entity is "Ground_Station.hwdef";
end Ground_Station;

architecture STRUCTURE of Ground_Station is
  component Ground_Station_zynq_ultra_ps_e_0_0 is
  port (
    emio_uart1_txd : out STD_LOGIC;
    emio_uart1_rxd : in STD_LOGIC
  );
  end component Ground_Station_zynq_ultra_ps_e_0_0;
  attribute X_INTERFACE_INFO : string;
  attribute X_INTERFACE_INFO of BLE_TX : signal is "xilinx.com:signal:data:1.0 DATA.BLE_TX DATA";
  attribute X_INTERFACE_PARAMETER : string;
  attribute X_INTERFACE_PARAMETER of BLE_TX : signal is "XIL_INTERFACENAME DATA.BLE_TX, LAYERED_METADATA undef";
  attribute X_INTERFACE_INFO of BLE_RST : signal is "xilinx.com:signal:data:1.0 DATA.BLE_RST DATA";
  attribute X_INTERFACE_PARAMETER of BLE_RST : signal is "XIL_INTERFACENAME DATA.BLE_RST, LAYERED_METADATA undef";
begin
  BLE_RST <= B"1";
zynq_ultra_ps_e_0: component Ground_Station_zynq_ultra_ps_e_0_0
     port map (
      emio_uart1_rxd => BLE_TX,
      emio_uart1_txd => BLE_RX
    );
end STRUCTURE;
