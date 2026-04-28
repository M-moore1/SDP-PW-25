transcript off
onbreak {quit -force}
onerror {quit -force}
transcript on

asim +access +r +m+Ground_Station  -L xil_defaultlib -L xilinx_vip -L axi_infrastructure_v1_1_0 -L axi_vip_v1_1_21 -L zynq_ultra_ps_e_vip_v1_0_21 -L xilinx_vip -L unisims_ver -L unimacro_ver -L secureip -O5 xil_defaultlib.Ground_Station xil_defaultlib.glbl

do {Ground_Station.udo}

run 1000ns

endsim

quit -force
