vlib questa_lib/work
vlib questa_lib/msim

vlib questa_lib/msim/xilinx_vip
vlib questa_lib/msim/axi_infrastructure_v1_1_0
vlib questa_lib/msim/axi_vip_v1_1_21
vlib questa_lib/msim/zynq_ultra_ps_e_vip_v1_0_21
vlib questa_lib/msim/xil_defaultlib

vmap xilinx_vip questa_lib/msim/xilinx_vip
vmap axi_infrastructure_v1_1_0 questa_lib/msim/axi_infrastructure_v1_1_0
vmap axi_vip_v1_1_21 questa_lib/msim/axi_vip_v1_1_21
vmap zynq_ultra_ps_e_vip_v1_0_21 questa_lib/msim/zynq_ultra_ps_e_vip_v1_0_21
vmap xil_defaultlib questa_lib/msim/xil_defaultlib

vlog -work xilinx_vip  -incr -mfcu  -sv -L axi_vip_v1_1_21 -L zynq_ultra_ps_e_vip_v1_0_21 -L xilinx_vip "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_axi4streampc.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi_vip_axi4pc.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/xil_common_vip_pkg.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_pkg.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi_vip_pkg.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_if.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi_vip_if.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/clk_vip_if.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/rst_vip_if.sv" \

vlog -work axi_infrastructure_v1_1_0  -incr -mfcu  "+incdir+../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/ec67/hdl" "+incdir+../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" \
"../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/ec67/hdl/axi_infrastructure_v1_1_vl_rfs.v" \

vlog -work axi_vip_v1_1_21  -incr -mfcu  -sv -L axi_vip_v1_1_21 -L zynq_ultra_ps_e_vip_v1_0_21 -L xilinx_vip "+incdir+../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/ec67/hdl" "+incdir+../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" \
"../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/f16f/hdl/axi_vip_v1_1_vl_rfs.sv" \

vlog -work zynq_ultra_ps_e_vip_v1_0_21  -incr -mfcu  -sv -L axi_vip_v1_1_21 -L zynq_ultra_ps_e_vip_v1_0_21 -L xilinx_vip "+incdir+../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/ec67/hdl" "+incdir+../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" \
"../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/7711/hdl/zynq_ultra_ps_e_vip_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -incr -mfcu  "+incdir+../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/ec67/hdl" "+incdir+../../../../GroundStation_BLE.gen/sources_1/bd/Ground_Station/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" \
"../../../bd/Ground_Station/ip/Ground_Station_zynq_ultra_ps_e_0_0/sim/Ground_Station_zynq_ultra_ps_e_0_0_vip_wrapper.v" \
"../../../bd/Ground_Station/sim/Ground_Station.v" \

vlog -work xil_defaultlib \
"glbl.v"

