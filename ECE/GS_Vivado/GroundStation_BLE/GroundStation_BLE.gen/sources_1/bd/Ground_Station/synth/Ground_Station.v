//Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
//Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2025.1 (win64) Build 6140274 Thu May 22 00:12:29 MDT 2025
//Date        : Fri Feb 27 17:17:06 2026
//Host        : SDP2635 running 64-bit major release  (build 9200)
//Command     : generate_target Ground_Station.bd
//Design      : Ground_Station
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

(* CORE_GENERATION_INFO = "Ground_Station,IP_Integrator,{x_ipVendor=xilinx.com,x_ipLibrary=BlockDiagram,x_ipName=Ground_Station,x_ipVersion=1.00.a,x_ipLanguage=VERILOG,numBlks=2,numReposBlks=2,numNonXlnxBlks=0,numHierBlks=0,maxHierDepth=0,numSysgenBlks=0,numHlsBlks=0,numHdlrefBlks=0,numPkgbdBlks=0,bdsource=USER,da_zynq_ultra_ps_e_cnt=1,synth_mode=Hierarchical}" *) (* HW_HANDOFF = "Ground_Station.hwdef" *) 
module Ground_Station
   (BLE_RST,
    BLE_RX,
    BLE_TX);
  (* X_INTERFACE_INFO = "xilinx.com:signal:data:1.0 DATA.BLE_RST DATA" *) (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME DATA.BLE_RST, LAYERED_METADATA undef" *) output [0:0]BLE_RST;
  output BLE_RX;
  (* X_INTERFACE_INFO = "xilinx.com:signal:data:1.0 DATA.BLE_TX DATA" *) (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME DATA.BLE_TX, LAYERED_METADATA undef" *) input BLE_TX;

  wire [0:0]BLE_RST;
  wire BLE_RX;
  wire BLE_TX;

  assign BLE_RST = 1'h1;
  Ground_Station_zynq_ultra_ps_e_0_0 zynq_ultra_ps_e_0
       (.emio_uart1_rxd(BLE_TX),
        .emio_uart1_txd(BLE_RX));
endmodule
