--Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
--Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
----------------------------------------------------------------------------------
--Tool Version: Vivado v.2025.1.1 (win64) Build 6233196 Thu Sep 11 21:27:30 MDT 2025
--Date        : Tue Dec  9 12:11:00 2025
--Host        : SDP2635 running 64-bit major release  (build 9200)
--Command     : generate_target ZCU102.bd
--Design      : ZCU102
--Purpose     : IP block netlist
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
library UNISIM;
use UNISIM.VCOMPONENTS.ALL;
entity ZCU102 is
  port (
    BT2_RX : out STD_LOGIC;
    BT2_TX : in STD_LOGIC
  );
  attribute CORE_GENERATION_INFO : string;
  attribute CORE_GENERATION_INFO of ZCU102 : entity is "ZCU102,IP_Integrator,{x_ipVendor=xilinx.com,x_ipLibrary=BlockDiagram,x_ipName=ZCU102,x_ipVersion=1.00.a,x_ipLanguage=VHDL,numBlks=1,numReposBlks=1,numNonXlnxBlks=0,numHierBlks=0,maxHierDepth=0,numSysgenBlks=0,numHlsBlks=0,numHdlrefBlks=0,numPkgbdBlks=0,bdsource=USER,da_zynq_ultra_ps_e_cnt=1,synth_mode=Hierarchical}";
  attribute HW_HANDOFF : string;
  attribute HW_HANDOFF of ZCU102 : entity is "ZCU102.hwdef";
end ZCU102;

architecture STRUCTURE of ZCU102 is
  component ZCU102_zynq_ultra_ps_e_0_0 is
  port (
    emio_uart1_txd : out STD_LOGIC;
    emio_uart1_rxd : in STD_LOGIC
  );
  end component ZCU102_zynq_ultra_ps_e_0_0;
  attribute X_INTERFACE_INFO : string;
  attribute X_INTERFACE_INFO of BT2_RX : signal is "xilinx.com:signal:data:1.0 DATA.BT2_RX DATA";
  attribute X_INTERFACE_PARAMETER : string;
  attribute X_INTERFACE_PARAMETER of BT2_RX : signal is "XIL_INTERFACENAME DATA.BT2_RX, LAYERED_METADATA undef";
  attribute X_INTERFACE_INFO of BT2_TX : signal is "xilinx.com:signal:data:1.0 DATA.BT2_TX DATA";
  attribute X_INTERFACE_PARAMETER of BT2_TX : signal is "XIL_INTERFACENAME DATA.BT2_TX, LAYERED_METADATA undef";
begin
zynq_ultra_ps_e_0: component ZCU102_zynq_ultra_ps_e_0_0
     port map (
      emio_uart1_rxd => BT2_TX,
      emio_uart1_txd => BT2_RX
    );
end STRUCTURE;
