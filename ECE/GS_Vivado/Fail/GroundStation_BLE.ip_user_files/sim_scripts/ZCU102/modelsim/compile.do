vlib modelsim_lib/work
vlib modelsim_lib/msim

vlib modelsim_lib/msim/xilinx_vip
vlib modelsim_lib/msim/axi_infrastructure_v1_1_0
vlib modelsim_lib/msim/axi_vip_v1_1_21
vlib modelsim_lib/msim/zynq_ultra_ps_e_vip_v1_0_21
vlib modelsim_lib/msim/xil_defaultlib

vmap xilinx_vip modelsim_lib/msim/xilinx_vip
vmap axi_infrastructure_v1_1_0 modelsim_lib/msim/axi_infrastructure_v1_1_0
vmap axi_vip_v1_1_21 modelsim_lib/msim/axi_vip_v1_1_21
vmap zynq_ultra_ps_e_vip_v1_0_21 modelsim_lib/msim/zynq_ultra_ps_e_vip_v1_0_21
vmap xil_defaultlib modelsim_lib/msim/xil_defaultlib

vlog -work xilinx_vip  -incr -mfcu  -sv -L axi_vip_v1_1_21 -L zynq_ultra_ps_e_vip_v1_0_21 -L xilinx_vip "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_axi4streampc.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi_vip_axi4pc.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/xil_common_vip_pkg.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_pkg.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi_vip_pkg.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_if.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi_vip_if.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/clk_vip_if.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/rst_vip_if.sv" \

vlog -work axi_infrastructure_v1_1_0  -incr -mfcu  "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl" "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" \
"../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl/axi_infrastructure_v1_1_vl_rfs.v" \

vlog -work axi_vip_v1_1_21  -incr -mfcu  -sv -L axi_vip_v1_1_21 -L zynq_ultra_ps_e_vip_v1_0_21 -L xilinx_vip "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl" "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" \
"../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/f16f/hdl/axi_vip_v1_1_vl_rfs.sv" \

vlog -work zynq_ultra_ps_e_vip_v1_0_21  -incr -mfcu  -sv -L axi_vip_v1_1_21 -L zynq_ultra_ps_e_vip_v1_0_21 -L xilinx_vip "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl" "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" \
"../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl/zynq_ultra_ps_e_vip_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -incr -mfcu  "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl" "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" \
"../../../bd/ZCU102/ip/ZCU102_zynq_ultra_ps_e_0_0/sim/ZCU102_zynq_ultra_ps_e_0_0_vip_wrapper.v" \

vcom -work xil_defaultlib  -93  \
"../../../bd/ZCU102/sim/ZCU102.vhd" \

vlog -work xil_defaultlib \
"glbl.v"

