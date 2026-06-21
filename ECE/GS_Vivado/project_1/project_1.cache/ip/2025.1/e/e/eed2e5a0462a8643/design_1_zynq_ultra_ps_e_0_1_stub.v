// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2025.1 (win64) Build 6140274 Thu May 22 00:12:29 MDT 2025
// Date        : Sat Mar 21 13:54:16 2026
// Host        : SDP2635 running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub -rename_top decalper_eb_ot_sdeen_pot_pi_dehcac_xnilix -prefix
//               decalper_eb_ot_sdeen_pot_pi_dehcac_xnilix_ design_1_zynq_ultra_ps_e_0_1_stub.v
// Design      : design_1_zynq_ultra_ps_e_0_1
// Purpose     : Stub declaration of top-level module interface
// Device      : xczu9eg-ffvb1156-2-e
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* CHECK_LICENSE_TYPE = "design_1_zynq_ultra_ps_e_0_1,zynq_ultra_ps_e_v3_5_7_zynq_ultra_ps_e,{}" *) (* core_generation_info = "design_1_zynq_ultra_ps_e_0_1,zynq_ultra_ps_e_v3_5_7_zynq_ultra_ps_e,{x_ipProduct=Vivado 2025.1,x_ipVendor=xilinx.com,x_ipLibrary=ip,x_ipName=zynq_ultra_ps_e,x_ipVersion=3.5,x_ipCoreRevision=7,x_ipLanguage=VHDL,x_ipSimLanguage=MIXED,C_DP_USE_AUDIO=0,C_DP_USE_VIDEO=0,C_MAXIGP0_DATA_WIDTH=128,C_MAXIGP1_DATA_WIDTH=128,C_MAXIGP2_DATA_WIDTH=32,C_SAXIGP0_DATA_WIDTH=128,C_SAXIGP1_DATA_WIDTH=128,C_SAXIGP2_DATA_WIDTH=128,C_SAXIGP3_DATA_WIDTH=128,C_SAXIGP4_DATA_WIDTH=128,C_SAXIGP5_DATA_WIDTH=128,C_SAXIGP6_DATA_WIDTH=128,C_USE_DIFF_RW_CLK_GP0=0,C_USE_DIFF_RW_CLK_GP1=0,C_USE_DIFF_RW_CLK_GP2=0,C_USE_DIFF_RW_CLK_GP3=0,C_USE_DIFF_RW_CLK_GP4=0,C_USE_DIFF_RW_CLK_GP5=0,C_USE_DIFF_RW_CLK_GP6=0,C_EN_FIFO_ENET0=0,C_EN_FIFO_ENET1=0,C_EN_FIFO_ENET2=0,C_EN_FIFO_ENET3=0,C_PL_CLK0_BUF=TRUE,C_PL_CLK1_BUF=FALSE,C_PL_CLK2_BUF=FALSE,C_PL_CLK3_BUF=FALSE,C_TRACE_PIPELINE_WIDTH=8,C_EN_EMIO_TRACE=0,C_TRACE_DATA_WIDTH=32,C_USE_DEBUG_TEST=0,C_SD0_INTERNAL_BUS_WIDTH=5,C_SD1_INTERNAL_BUS_WIDTH=8,C_NUM_F2P_0_INTR_INPUTS=1,C_NUM_F2P_1_INTR_INPUTS=1,C_EMIO_GPIO_WIDTH=95,C_NUM_FABRIC_RESETS=1}" *) (* downgradeipidentifiedwarnings = "yes" *) 
(* x_core_info = "zynq_ultra_ps_e_v3_5_7_zynq_ultra_ps_e,Vivado 2025.1" *) 
module decalper_eb_ot_sdeen_pot_pi_dehcac_xnilix(maxihpm0_fpd_aclk, maxigp0_awid, 
  maxigp0_awaddr, maxigp0_awlen, maxigp0_awsize, maxigp0_awburst, maxigp0_awlock, 
  maxigp0_awcache, maxigp0_awprot, maxigp0_awvalid, maxigp0_awuser, maxigp0_awready, 
  maxigp0_wdata, maxigp0_wstrb, maxigp0_wlast, maxigp0_wvalid, maxigp0_wready, maxigp0_bid, 
  maxigp0_bresp, maxigp0_bvalid, maxigp0_bready, maxigp0_arid, maxigp0_araddr, maxigp0_arlen, 
  maxigp0_arsize, maxigp0_arburst, maxigp0_arlock, maxigp0_arcache, maxigp0_arprot, 
  maxigp0_arvalid, maxigp0_aruser, maxigp0_arready, maxigp0_rid, maxigp0_rdata, 
  maxigp0_rresp, maxigp0_rlast, maxigp0_rvalid, maxigp0_rready, maxigp0_awqos, maxigp0_arqos, 
  maxihpm1_fpd_aclk, maxigp1_awid, maxigp1_awaddr, maxigp1_awlen, maxigp1_awsize, 
  maxigp1_awburst, maxigp1_awlock, maxigp1_awcache, maxigp1_awprot, maxigp1_awvalid, 
  maxigp1_awuser, maxigp1_awready, maxigp1_wdata, maxigp1_wstrb, maxigp1_wlast, 
  maxigp1_wvalid, maxigp1_wready, maxigp1_bid, maxigp1_bresp, maxigp1_bvalid, maxigp1_bready, 
  maxigp1_arid, maxigp1_araddr, maxigp1_arlen, maxigp1_arsize, maxigp1_arburst, 
  maxigp1_arlock, maxigp1_arcache, maxigp1_arprot, maxigp1_arvalid, maxigp1_aruser, 
  maxigp1_arready, maxigp1_rid, maxigp1_rdata, maxigp1_rresp, maxigp1_rlast, maxigp1_rvalid, 
  maxigp1_rready, maxigp1_awqos, maxigp1_arqos, emio_uart1_txd, emio_uart1_rxd, 
  emio_uart1_ctsn, emio_uart1_rtsn, emio_uart1_dsrn, emio_uart1_dcdn, emio_uart1_rin, 
  emio_uart1_dtrn, pl_resetn0, pl_clk0)
/* synthesis syn_black_box black_box_pad_pin="maxigp0_awid[15:0],maxigp0_awaddr[39:0],maxigp0_awlen[7:0],maxigp0_awsize[2:0],maxigp0_awburst[1:0],maxigp0_awlock,maxigp0_awcache[3:0],maxigp0_awprot[2:0],maxigp0_awvalid,maxigp0_awuser[15:0],maxigp0_awready,maxigp0_wdata[127:0],maxigp0_wstrb[15:0],maxigp0_wlast,maxigp0_wvalid,maxigp0_wready,maxigp0_bid[15:0],maxigp0_bresp[1:0],maxigp0_bvalid,maxigp0_bready,maxigp0_arid[15:0],maxigp0_araddr[39:0],maxigp0_arlen[7:0],maxigp0_arsize[2:0],maxigp0_arburst[1:0],maxigp0_arlock,maxigp0_arcache[3:0],maxigp0_arprot[2:0],maxigp0_arvalid,maxigp0_aruser[15:0],maxigp0_arready,maxigp0_rid[15:0],maxigp0_rdata[127:0],maxigp0_rresp[1:0],maxigp0_rlast,maxigp0_rvalid,maxigp0_rready,maxigp0_awqos[3:0],maxigp0_arqos[3:0],maxigp1_awid[15:0],maxigp1_awaddr[39:0],maxigp1_awlen[7:0],maxigp1_awsize[2:0],maxigp1_awburst[1:0],maxigp1_awlock,maxigp1_awcache[3:0],maxigp1_awprot[2:0],maxigp1_awvalid,maxigp1_awuser[15:0],maxigp1_awready,maxigp1_wdata[127:0],maxigp1_wstrb[15:0],maxigp1_wlast,maxigp1_wvalid,maxigp1_wready,maxigp1_bid[15:0],maxigp1_bresp[1:0],maxigp1_bvalid,maxigp1_bready,maxigp1_arid[15:0],maxigp1_araddr[39:0],maxigp1_arlen[7:0],maxigp1_arsize[2:0],maxigp1_arburst[1:0],maxigp1_arlock,maxigp1_arcache[3:0],maxigp1_arprot[2:0],maxigp1_arvalid,maxigp1_aruser[15:0],maxigp1_arready,maxigp1_rid[15:0],maxigp1_rdata[127:0],maxigp1_rresp[1:0],maxigp1_rlast,maxigp1_rvalid,maxigp1_rready,maxigp1_awqos[3:0],maxigp1_arqos[3:0],emio_uart1_txd,emio_uart1_rxd,emio_uart1_ctsn,emio_uart1_rtsn,emio_uart1_dsrn,emio_uart1_dcdn,emio_uart1_rin,emio_uart1_dtrn,pl_resetn0" */
/* synthesis syn_force_seq_prim="maxihpm0_fpd_aclk" */
/* synthesis syn_force_seq_prim="maxihpm1_fpd_aclk" */
/* synthesis syn_force_seq_prim="pl_clk0" */;
  (* x_interface_info = "xilinx.com:signal:clock:1.0 M_AXI_HPM0_FPD_ACLK CLK" *) (* x_interface_mode = "slave M_AXI_HPM0_FPD_ACLK" *) (* x_interface_parameter = "XIL_INTERFACENAME M_AXI_HPM0_FPD_ACLK, ASSOCIATED_BUSIF M_AXI_HPM0_FPD, FREQ_HZ 99990005, FREQ_TOLERANCE_HZ 0, PHASE 0.0, CLK_DOMAIN design_1_zynq_ultra_ps_e_0_1_pl_clk0, INSERT_VIP 0" *) input maxihpm0_fpd_aclk /* synthesis syn_isclock = 1 */;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWID" *) (* x_interface_mode = "master M_AXI_HPM0_FPD" *) (* x_interface_parameter = "XIL_INTERFACENAME M_AXI_HPM0_FPD, NUM_WRITE_OUTSTANDING 8, NUM_READ_OUTSTANDING 8, DATA_WIDTH 128, PROTOCOL AXI4, FREQ_HZ 99990005, ID_WIDTH 16, ADDR_WIDTH 40, AWUSER_WIDTH 16, ARUSER_WIDTH 16, WUSER_WIDTH 0, RUSER_WIDTH 0, BUSER_WIDTH 0, READ_WRITE_MODE READ_WRITE, HAS_BURST 1, HAS_LOCK 1, HAS_PROT 1, HAS_CACHE 1, HAS_QOS 1, HAS_REGION 0, HAS_WSTRB 1, HAS_BRESP 1, HAS_RRESP 1, SUPPORTS_NARROW_BURST 1, MAX_BURST_LENGTH 256, PHASE 0.0, CLK_DOMAIN design_1_zynq_ultra_ps_e_0_1_pl_clk0, NUM_READ_THREADS 4, NUM_WRITE_THREADS 4, RUSER_BITS_PER_BYTE 0, WUSER_BITS_PER_BYTE 0, INSERT_VIP 0" *) output [15:0]maxigp0_awid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWADDR" *) output [39:0]maxigp0_awaddr;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWLEN" *) output [7:0]maxigp0_awlen;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWSIZE" *) output [2:0]maxigp0_awsize;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWBURST" *) output [1:0]maxigp0_awburst;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWLOCK" *) output maxigp0_awlock;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWCACHE" *) output [3:0]maxigp0_awcache;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWPROT" *) output [2:0]maxigp0_awprot;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWVALID" *) output maxigp0_awvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWUSER" *) output [15:0]maxigp0_awuser;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWREADY" *) input maxigp0_awready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD WDATA" *) output [127:0]maxigp0_wdata;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD WSTRB" *) output [15:0]maxigp0_wstrb;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD WLAST" *) output maxigp0_wlast;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD WVALID" *) output maxigp0_wvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD WREADY" *) input maxigp0_wready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD BID" *) input [15:0]maxigp0_bid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD BRESP" *) input [1:0]maxigp0_bresp;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD BVALID" *) input maxigp0_bvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD BREADY" *) output maxigp0_bready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARID" *) output [15:0]maxigp0_arid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARADDR" *) output [39:0]maxigp0_araddr;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARLEN" *) output [7:0]maxigp0_arlen;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARSIZE" *) output [2:0]maxigp0_arsize;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARBURST" *) output [1:0]maxigp0_arburst;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARLOCK" *) output maxigp0_arlock;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARCACHE" *) output [3:0]maxigp0_arcache;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARPROT" *) output [2:0]maxigp0_arprot;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARVALID" *) output maxigp0_arvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARUSER" *) output [15:0]maxigp0_aruser;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARREADY" *) input maxigp0_arready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD RID" *) input [15:0]maxigp0_rid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD RDATA" *) input [127:0]maxigp0_rdata;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD RRESP" *) input [1:0]maxigp0_rresp;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD RLAST" *) input maxigp0_rlast;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD RVALID" *) input maxigp0_rvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD RREADY" *) output maxigp0_rready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD AWQOS" *) output [3:0]maxigp0_awqos;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM0_FPD ARQOS" *) output [3:0]maxigp0_arqos;
  (* x_interface_info = "xilinx.com:signal:clock:1.0 M_AXI_HPM1_FPD_ACLK CLK" *) (* x_interface_mode = "slave M_AXI_HPM1_FPD_ACLK" *) (* x_interface_parameter = "XIL_INTERFACENAME M_AXI_HPM1_FPD_ACLK, ASSOCIATED_BUSIF M_AXI_HPM1_FPD, FREQ_HZ 99990005, FREQ_TOLERANCE_HZ 0, PHASE 0.0, CLK_DOMAIN design_1_zynq_ultra_ps_e_0_1_pl_clk0, INSERT_VIP 0" *) input maxihpm1_fpd_aclk /* synthesis syn_isclock = 1 */;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWID" *) (* x_interface_mode = "master M_AXI_HPM1_FPD" *) (* x_interface_parameter = "XIL_INTERFACENAME M_AXI_HPM1_FPD, NUM_WRITE_OUTSTANDING 8, NUM_READ_OUTSTANDING 8, DATA_WIDTH 128, PROTOCOL AXI4, FREQ_HZ 99990005, ID_WIDTH 16, ADDR_WIDTH 40, AWUSER_WIDTH 16, ARUSER_WIDTH 16, WUSER_WIDTH 0, RUSER_WIDTH 0, BUSER_WIDTH 0, READ_WRITE_MODE READ_WRITE, HAS_BURST 1, HAS_LOCK 1, HAS_PROT 1, HAS_CACHE 1, HAS_QOS 1, HAS_REGION 0, HAS_WSTRB 1, HAS_BRESP 1, HAS_RRESP 1, SUPPORTS_NARROW_BURST 1, MAX_BURST_LENGTH 256, PHASE 0.0, CLK_DOMAIN design_1_zynq_ultra_ps_e_0_1_pl_clk0, NUM_READ_THREADS 4, NUM_WRITE_THREADS 4, RUSER_BITS_PER_BYTE 0, WUSER_BITS_PER_BYTE 0, INSERT_VIP 0" *) output [15:0]maxigp1_awid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWADDR" *) output [39:0]maxigp1_awaddr;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWLEN" *) output [7:0]maxigp1_awlen;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWSIZE" *) output [2:0]maxigp1_awsize;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWBURST" *) output [1:0]maxigp1_awburst;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWLOCK" *) output maxigp1_awlock;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWCACHE" *) output [3:0]maxigp1_awcache;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWPROT" *) output [2:0]maxigp1_awprot;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWVALID" *) output maxigp1_awvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWUSER" *) output [15:0]maxigp1_awuser;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWREADY" *) input maxigp1_awready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD WDATA" *) output [127:0]maxigp1_wdata;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD WSTRB" *) output [15:0]maxigp1_wstrb;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD WLAST" *) output maxigp1_wlast;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD WVALID" *) output maxigp1_wvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD WREADY" *) input maxigp1_wready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD BID" *) input [15:0]maxigp1_bid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD BRESP" *) input [1:0]maxigp1_bresp;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD BVALID" *) input maxigp1_bvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD BREADY" *) output maxigp1_bready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARID" *) output [15:0]maxigp1_arid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARADDR" *) output [39:0]maxigp1_araddr;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARLEN" *) output [7:0]maxigp1_arlen;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARSIZE" *) output [2:0]maxigp1_arsize;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARBURST" *) output [1:0]maxigp1_arburst;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARLOCK" *) output maxigp1_arlock;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARCACHE" *) output [3:0]maxigp1_arcache;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARPROT" *) output [2:0]maxigp1_arprot;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARVALID" *) output maxigp1_arvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARUSER" *) output [15:0]maxigp1_aruser;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARREADY" *) input maxigp1_arready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD RID" *) input [15:0]maxigp1_rid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD RDATA" *) input [127:0]maxigp1_rdata;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD RRESP" *) input [1:0]maxigp1_rresp;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD RLAST" *) input maxigp1_rlast;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD RVALID" *) input maxigp1_rvalid;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD RREADY" *) output maxigp1_rready;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD AWQOS" *) output [3:0]maxigp1_awqos;
  (* x_interface_info = "xilinx.com:interface:aximm:1.0 M_AXI_HPM1_FPD ARQOS" *) output [3:0]maxigp1_arqos;
  (* x_interface_info = "xilinx.com:interface:uart:1.0 UART_1 TxD" *) (* x_interface_mode = "master UART_1" *) output emio_uart1_txd;
  (* x_interface_info = "xilinx.com:interface:uart:1.0 UART_1 RxD" *) input emio_uart1_rxd;
  (* x_interface_info = "xilinx.com:interface:uart:1.0 UART_1 CTSn" *) input emio_uart1_ctsn;
  (* x_interface_info = "xilinx.com:interface:uart:1.0 UART_1 RTSn" *) output emio_uart1_rtsn;
  (* x_interface_info = "xilinx.com:interface:uart:1.0 UART_1 DSRn" *) input emio_uart1_dsrn;
  (* x_interface_info = "xilinx.com:interface:uart:1.0 UART_1 DCDn" *) input emio_uart1_dcdn;
  (* x_interface_info = "xilinx.com:interface:uart:1.0 UART_1 RI" *) input emio_uart1_rin;
  (* x_interface_info = "xilinx.com:interface:uart:1.0 UART_1 DTRn" *) output emio_uart1_dtrn;
  (* x_interface_info = "xilinx.com:signal:reset:1.0 PL_RESETN0 RST" *) (* x_interface_mode = "master PL_RESETN0" *) (* x_interface_parameter = "XIL_INTERFACENAME PL_RESETN0, POLARITY ACTIVE_LOW, INSERT_VIP 0" *) output pl_resetn0;
  (* x_interface_info = "xilinx.com:signal:clock:1.0 PL_CLK0 CLK" *) (* x_interface_mode = "master PL_CLK0" *) (* x_interface_parameter = "XIL_INTERFACENAME PL_CLK0, FREQ_HZ 99990005, FREQ_TOLERANCE_HZ 0, PHASE 0.0, CLK_DOMAIN design_1_zynq_ultra_ps_e_0_1_pl_clk0, INSERT_VIP 0" *) output pl_clk0 /* synthesis syn_isclock = 1 */;
endmodule
