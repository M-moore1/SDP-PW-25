transcript off
onbreak {quit -force}
onerror {quit -force}
transcript on

vlib work
vlib activehdl/xilinx_vip
vlib activehdl/axi_infrastructure_v1_1_0
vlib activehdl/axi_vip_v1_1_21
vlib activehdl/zynq_ultra_ps_e_vip_v1_0_21
vlib activehdl/xil_defaultlib

vmap xilinx_vip activehdl/xilinx_vip
vmap axi_infrastructure_v1_1_0 activehdl/axi_infrastructure_v1_1_0
vmap axi_vip_v1_1_21 activehdl/axi_vip_v1_1_21
vmap zynq_ultra_ps_e_vip_v1_0_21 activehdl/zynq_ultra_ps_e_vip_v1_0_21
vmap xil_defaultlib activehdl/xil_defaultlib

vlog -work xilinx_vip  -sv2k12 "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l xil_defaultlib \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_axi4streampc.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi_vip_axi4pc.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/xil_common_vip_pkg.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_pkg.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi_vip_pkg.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_if.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/axi_vip_if.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/clk_vip_if.sv" \
"C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/hdl/rst_vip_if.sv" \

vlog -work axi_infrastructure_v1_1_0  -v2k5 "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl" "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l xil_defaultlib \
"../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl/axi_infrastructure_v1_1_vl_rfs.v" \

vlog -work axi_vip_v1_1_21  -sv2k12 "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl" "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l xil_defaultlib \
"../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/f16f/hdl/axi_vip_v1_1_vl_rfs.sv" \

vlog -work zynq_ultra_ps_e_vip_v1_0_21  -sv2k12 "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl" "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l xil_defaultlib \
"../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl/zynq_ultra_ps_e_vip_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/ec67/hdl" "+incdir+../../../../pmod_ip.gen/sources_1/bd/ZCU102/ipshared/7711/hdl" "+incdir+../../../../../../../../../Xilinx/2025.1.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l xil_defaultlib \
"../../../bd/ZCU102/ip/ZCU102_zynq_ultra_ps_e_0_0/sim/ZCU102_zynq_ultra_ps_e_0_0_vip_wrapper.v" \

vcom -work xil_defaultlib -93  \
"../../../bd/ZCU102/sim/ZCU102.vhd" \

vlog -work xil_defaultlib \
"glbl.v"

