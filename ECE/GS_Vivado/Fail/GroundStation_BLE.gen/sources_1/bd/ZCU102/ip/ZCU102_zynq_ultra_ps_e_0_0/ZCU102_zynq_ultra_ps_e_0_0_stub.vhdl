-- Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
-- Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
-- --------------------------------------------------------------------------------
-- Tool Version: Vivado v.2025.1.1 (win64) Build 6233196 Thu Sep 11 21:27:30 MDT 2025
-- Date        : Tue Dec  9 12:12:43 2025
-- Host        : SDP2635 running 64-bit major release  (build 9200)
-- Command     : write_vhdl -force -mode synth_stub
--               c:/Users/ist21005/Documents/Board_Practice/pmod_ip/pmod_ip.gen/sources_1/bd/ZCU102/ip/ZCU102_zynq_ultra_ps_e_0_0/ZCU102_zynq_ultra_ps_e_0_0_stub.vhdl
-- Design      : ZCU102_zynq_ultra_ps_e_0_0
-- Purpose     : Stub declaration of top-level module interface
-- Device      : xczu9eg-ffvb1156-2-e
-- --------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

entity ZCU102_zynq_ultra_ps_e_0_0 is
  Port ( 
    emio_uart1_txd : out STD_LOGIC;
    emio_uart1_rxd : in STD_LOGIC
  );

  attribute CHECK_LICENSE_TYPE : string;
  attribute CHECK_LICENSE_TYPE of ZCU102_zynq_ultra_ps_e_0_0 : entity is "ZCU102_zynq_ultra_ps_e_0_0,zynq_ultra_ps_e_v3_5_7_zynq_ultra_ps_e,{}";
  attribute core_generation_info : string;
  attribute core_generation_info of ZCU102_zynq_ultra_ps_e_0_0 : entity is "ZCU102_zynq_ultra_ps_e_0_0,zynq_ultra_ps_e_v3_5_7_zynq_ultra_ps_e,{x_ipProduct=Vivado 2025.1.1,x_ipVendor=xilinx.com,x_ipLibrary=ip,x_ipName=zynq_ultra_ps_e,x_ipVersion=3.5,x_ipCoreRevision=7,x_ipLanguage=VHDL,x_ipSimLanguage=MIXED,C_DP_USE_AUDIO=0,C_DP_USE_VIDEO=0,C_MAXIGP0_DATA_WIDTH=128,C_MAXIGP1_DATA_WIDTH=128,C_MAXIGP2_DATA_WIDTH=32,C_SAXIGP0_DATA_WIDTH=128,C_SAXIGP1_DATA_WIDTH=128,C_SAXIGP2_DATA_WIDTH=128,C_SAXIGP3_DATA_WIDTH=128,C_SAXIGP4_DATA_WIDTH=128,C_SAXIGP5_DATA_WIDTH=128,C_SAXIGP6_DATA_WIDTH=128,C_USE_DIFF_RW_CLK_GP0=0,C_USE_DIFF_RW_CLK_GP1=0,C_USE_DIFF_RW_CLK_GP2=0,C_USE_DIFF_RW_CLK_GP3=0,C_USE_DIFF_RW_CLK_GP4=0,C_USE_DIFF_RW_CLK_GP5=0,C_USE_DIFF_RW_CLK_GP6=0,C_EN_FIFO_ENET0=0,C_EN_FIFO_ENET1=0,C_EN_FIFO_ENET2=0,C_EN_FIFO_ENET3=0,C_PL_CLK0_BUF=FALSE,C_PL_CLK1_BUF=FALSE,C_PL_CLK2_BUF=FALSE,C_PL_CLK3_BUF=FALSE,C_TRACE_PIPELINE_WIDTH=8,C_EN_EMIO_TRACE=0,C_TRACE_DATA_WIDTH=32,C_USE_DEBUG_TEST=0,C_SD0_INTERNAL_BUS_WIDTH=5,C_SD1_INTERNAL_BUS_WIDTH=8,C_NUM_F2P_0_INTR_INPUTS=1,C_NUM_F2P_1_INTR_INPUTS=1,C_EMIO_GPIO_WIDTH=1,C_NUM_FABRIC_RESETS=0}";
  attribute downgradeipidentifiedwarnings : string;
  attribute downgradeipidentifiedwarnings of ZCU102_zynq_ultra_ps_e_0_0 : entity is "yes";
end ZCU102_zynq_ultra_ps_e_0_0;

architecture stub of ZCU102_zynq_ultra_ps_e_0_0 is
  attribute syn_black_box : boolean;
  attribute black_box_pad_pin : string;
  attribute syn_black_box of stub : architecture is true;
  attribute black_box_pad_pin of stub : architecture is "emio_uart1_txd,emio_uart1_rxd";
  attribute x_interface_info : string;
  attribute x_interface_info of emio_uart1_txd : signal is "xilinx.com:interface:uart:1.0 UART_1 TxD";
  attribute x_interface_mode : string;
  attribute x_interface_mode of emio_uart1_txd : signal is "master UART_1";
  attribute x_interface_info of emio_uart1_rxd : signal is "xilinx.com:interface:uart:1.0 UART_1 RxD";
  attribute x_core_info : string;
  attribute x_core_info of stub : architecture is "zynq_ultra_ps_e_v3_5_7_zynq_ultra_ps_e,Vivado 2025.1.1";
begin
end;
