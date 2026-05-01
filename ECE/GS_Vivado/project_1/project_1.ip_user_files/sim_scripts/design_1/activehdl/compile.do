transcript off
onbreak {quit -force}
onerror {quit -force}
transcript on

vlib work
vlib activehdl/xilinx_vip
vlib activehdl/xpm
vlib activehdl/axi_lite_ipif_v3_0_4
vlib activehdl/interrupt_control_v3_1_5
vlib activehdl/axi_gpio_v2_0_37
vlib activehdl/xil_defaultlib
vlib activehdl/axi_uartlite_v2_0_39
vlib activehdl/axi_infrastructure_v1_1_0
vlib activehdl/axi_vip_v1_1_21
vlib activehdl/zynq_ultra_ps_e_vip_v1_0_21
vlib activehdl/proc_sys_reset_v5_0_17
vlib activehdl/xlconstant_v1_1_10
vlib activehdl/smartconnect_v1_0
vlib activehdl/axi_register_slice_v2_1_35

vmap xilinx_vip activehdl/xilinx_vip
vmap xpm activehdl/xpm
vmap axi_lite_ipif_v3_0_4 activehdl/axi_lite_ipif_v3_0_4
vmap interrupt_control_v3_1_5 activehdl/interrupt_control_v3_1_5
vmap axi_gpio_v2_0_37 activehdl/axi_gpio_v2_0_37
vmap xil_defaultlib activehdl/xil_defaultlib
vmap axi_uartlite_v2_0_39 activehdl/axi_uartlite_v2_0_39
vmap axi_infrastructure_v1_1_0 activehdl/axi_infrastructure_v1_1_0
vmap axi_vip_v1_1_21 activehdl/axi_vip_v1_1_21
vmap zynq_ultra_ps_e_vip_v1_0_21 activehdl/zynq_ultra_ps_e_vip_v1_0_21
vmap proc_sys_reset_v5_0_17 activehdl/proc_sys_reset_v5_0_17
vmap xlconstant_v1_1_10 activehdl/xlconstant_v1_1_10
vmap smartconnect_v1_0 activehdl/smartconnect_v1_0
vmap axi_register_slice_v2_1_35 activehdl/axi_register_slice_v2_1_35

vlog -work xilinx_vip  -sv2k12 "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_axi4streampc.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi_vip_axi4pc.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/xil_common_vip_pkg.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_pkg.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi_vip_pkg.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi4stream_vip_if.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/axi_vip_if.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/clk_vip_if.sv" \
"C:/Xilinx/2025.1/Vivado/data/xilinx_vip/hdl/rst_vip_if.sv" \

vlog -work xpm  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"C:/Xilinx/2025.1/Vivado/data/ip/xpm/xpm_cdc/hdl/xpm_cdc.sv" \
"C:/Xilinx/2025.1/Vivado/data/ip/xpm/xpm_fifo/hdl/xpm_fifo.sv" \
"C:/Xilinx/2025.1/Vivado/data/ip/xpm/xpm_memory/hdl/xpm_memory.sv" \

vcom -work xpm -93  \
"C:/Xilinx/2025.1/Vivado/data/ip/xpm/xpm_VCOMP.vhd" \

vcom -work axi_lite_ipif_v3_0_4 -93  \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/66ea/hdl/axi_lite_ipif_v3_0_vh_rfs.vhd" \

vcom -work interrupt_control_v3_1_5 -93  \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/d8cc/hdl/interrupt_control_v3_1_vh_rfs.vhd" \

vcom -work axi_gpio_v2_0_37 -93  \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/0271/hdl/axi_gpio_v2_0_vh_rfs.vhd" \

vcom -work xil_defaultlib -93  \
"../../../bd/design_1/ip/design_1_axi_gpio_0_0/sim/design_1_axi_gpio_0_0.vhd" \

vcom -work axi_uartlite_v2_0_39 -93  \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/eab1/hdl/axi_uartlite_v2_0_vh_rfs.vhd" \

vcom -work xil_defaultlib -93  \
"../../../bd/design_1/ip/design_1_axi_uartlite_0_0/sim/design_1_axi_uartlite_0_0.vhd" \
"../../../bd/design_1/ip/design_1_axi_gpio_0_1/sim/design_1_axi_gpio_0_1.vhd" \

vlog -work axi_infrastructure_v1_1_0  -v2k5 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl/axi_infrastructure_v1_1_vl_rfs.v" \

vlog -work axi_vip_v1_1_21  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/f16f/hdl/axi_vip_v1_1_vl_rfs.sv" \

vlog -work zynq_ultra_ps_e_vip_v1_0_21  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl/zynq_ultra_ps_e_vip_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_zynq_ultra_ps_e_0_1/sim/design_1_zynq_ultra_ps_e_0_1_vip_wrapper.v" \

vcom -work proc_sys_reset_v5_0_17 -93  \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/9438/hdl/proc_sys_reset_v5_0_vh_rfs.vhd" \

vcom -work xil_defaultlib -93  \
"../../../bd/design_1/ip/design_1_rst_ps8_0_99M_1/sim/design_1_rst_ps8_0_99M_1.vhd" \
"../../../bd/design_1/ipshared/defa/hdl/ADD_2_slave_lite_v1_0_S00_AXI.vhd" \
"../../../bd/design_1/ipshared/defa/hdl/ADD_2.vhd" \
"../../../bd/design_1/ipshared/defa/e2ba/simple_adder.vhd" \
"../../../bd/design_1/ip/design_1_ADD_2_0_0/sim/design_1_ADD_2_0_0.vhd" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/sim/bd_ad43.v" \

vlog -work xlconstant_v1_1_10  -v2k5 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/a165/hdl/xlconstant_v1_1_vl_rfs.v" \

vlog -work xil_defaultlib  -v2k5 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_0/sim/bd_ad43_one_0.v" \

vcom -work xil_defaultlib -93  \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_1/sim/bd_ad43_psr_aclk_0.vhd" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/sc_util_v1_0_vl_rfs.sv" \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/3718/hdl/sc_switchboard_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_2/sim/bd_ad43_arinsw_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_3/sim/bd_ad43_rinsw_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_4/sim/bd_ad43_awinsw_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_5/sim/bd_ad43_winsw_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_6/sim/bd_ad43_binsw_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_7/sim/bd_ad43_aroutsw_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_8/sim/bd_ad43_routsw_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_9/sim/bd_ad43_awoutsw_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_10/sim/bd_ad43_woutsw_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_11/sim/bd_ad43_boutsw_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/sc_node_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_12/sim/bd_ad43_arni_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_13/sim/bd_ad43_rni_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_14/sim/bd_ad43_awni_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_15/sim/bd_ad43_wni_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_16/sim/bd_ad43_bni_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/d800/hdl/sc_mmu_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_17/sim/bd_ad43_s00mmu_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/2da8/hdl/sc_transaction_regulator_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_18/sim/bd_ad43_s00tr_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/dce3/hdl/sc_si_converter_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_19/sim/bd_ad43_s00sic_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/cef3/hdl/sc_axi2sc_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_20/sim/bd_ad43_s00a2s_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_21/sim/bd_ad43_sarn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_22/sim/bd_ad43_srn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_23/sim/bd_ad43_sawn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_24/sim/bd_ad43_swn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_25/sim/bd_ad43_sbn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_26/sim/bd_ad43_s01mmu_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_27/sim/bd_ad43_s01tr_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_28/sim/bd_ad43_s01sic_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_29/sim/bd_ad43_s01a2s_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_30/sim/bd_ad43_sarn_1.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_31/sim/bd_ad43_srn_1.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_32/sim/bd_ad43_sawn_1.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_33/sim/bd_ad43_swn_1.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_34/sim/bd_ad43_sbn_1.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/7f4f/hdl/sc_sc2axi_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_35/sim/bd_ad43_m00s2a_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_36/sim/bd_ad43_m00arn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_37/sim/bd_ad43_m00rn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_38/sim/bd_ad43_m00awn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_39/sim/bd_ad43_m00wn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_40/sim/bd_ad43_m00bn_0.sv" \

vlog -work smartconnect_v1_0  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/0133/hdl/sc_exit_v1_0_vl_rfs.sv" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_41/sim/bd_ad43_m00e_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_42/sim/bd_ad43_m01s2a_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_43/sim/bd_ad43_m01arn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_44/sim/bd_ad43_m01rn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_45/sim/bd_ad43_m01awn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_46/sim/bd_ad43_m01wn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_47/sim/bd_ad43_m01bn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_48/sim/bd_ad43_m01e_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_49/sim/bd_ad43_m02s2a_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_50/sim/bd_ad43_m02arn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_51/sim/bd_ad43_m02rn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_52/sim/bd_ad43_m02awn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_53/sim/bd_ad43_m02wn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_54/sim/bd_ad43_m02bn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_55/sim/bd_ad43_m02e_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_56/sim/bd_ad43_m03s2a_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_57/sim/bd_ad43_m03arn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_58/sim/bd_ad43_m03rn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_59/sim/bd_ad43_m03awn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_60/sim/bd_ad43_m03wn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_61/sim/bd_ad43_m03bn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_62/sim/bd_ad43_m03e_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_63/sim/bd_ad43_m04s2a_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_64/sim/bd_ad43_m04arn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_65/sim/bd_ad43_m04rn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_66/sim/bd_ad43_m04awn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_67/sim/bd_ad43_m04wn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_68/sim/bd_ad43_m04bn_0.sv" \
"../../../bd/design_1/ip/design_1_axi_smc_6/bd_0/ip/ip_69/sim/bd_ad43_m04e_0.sv" \

vlog -work axi_register_slice_v2_1_35  -v2k5 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../../project_1.gen/sources_1/bd/design_1/ipshared/c5b7/hdl/axi_register_slice_v2_1_vl_rfs.v" \

vlog -work xil_defaultlib  -sv2k12 "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/ec67/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/7711/hdl" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/f0b6/hdl/verilog" "+incdir+../../../../project_1.gen/sources_1/bd/design_1/ipshared/a8e4/hdl/verilog" "+incdir+../../../../../../../../../Xilinx/2025.1/Vivado/data/rsb/busdef" "+incdir+C:/Xilinx/2025.1/Vivado/data/xilinx_vip/include" -l xilinx_vip -l xpm -l axi_lite_ipif_v3_0_4 -l interrupt_control_v3_1_5 -l axi_gpio_v2_0_37 -l xil_defaultlib -l axi_uartlite_v2_0_39 -l axi_infrastructure_v1_1_0 -l axi_vip_v1_1_21 -l zynq_ultra_ps_e_vip_v1_0_21 -l proc_sys_reset_v5_0_17 -l xlconstant_v1_1_10 -l smartconnect_v1_0 -l axi_register_slice_v2_1_35 \
"../../../bd/design_1/ip/design_1_axi_smc_6/sim/design_1_axi_smc_6.sv" \

vcom -work xil_defaultlib -93  \
"../../../bd/design_1/ipshared/b032/hdl/AXI_AES_GCM_slave_lite_v1_0_S00_AXI.vhd" \
"../../../bd/design_1/ipshared/b032/src/aes_pkg.vhd" \
"../../../bd/design_1/ipshared/b032/src/gcm_pkg.vhd" \
"../../../bd/design_1/ipshared/b032/src/top_aes_gcm.vhd" \
"../../../bd/design_1/ipshared/b032/src/aes_func.vhd" \
"../../../bd/design_1/ipshared/b032/src/aes_ecb.vhd" \
"../../../bd/design_1/ipshared/b032/src/aes_enc_dec_ctrl.vhd" \
"../../../bd/design_1/ipshared/b032/src/aes_gcm.vhd" \
"../../../bd/design_1/ipshared/b032/src/aes_icb.vhd" \
"../../../bd/design_1/ipshared/b032/src/aes_kexp.vhd" \
"../../../bd/design_1/ipshared/b032/src/aes_last_round.vhd" \
"../../../bd/design_1/ipshared/b032/src/aes_round.vhd" \
"../../../bd/design_1/ipshared/b032/src/gcm_gctr.vhd" \
"../../../bd/design_1/ipshared/b032/src/gcm_ghash.vhd" \
"../../../bd/design_1/ipshared/b032/src/ghash_gfmul.vhd" \
"../../../bd/design_1/ipshared/b032/hdl/AXI_AES_GCM.vhd" \
"../../../bd/design_1/ip/design_1_AXI_AES_GCM_0_10/sim/design_1_AXI_AES_GCM_0_10.vhd" \
"../../../bd/design_1/sim/design_1.vhd" \

vlog -work xil_defaultlib \
"glbl.v"

