onbreak {quit -f}
onerror {quit -f}

vsim  -lib xil_defaultlib ZCU102_opt

set NumericStdNoWarnings 1
set StdArithNoWarnings 1

do {wave.do}

view wave
view structure
view signals

do {ZCU102.udo}

run 1000ns

quit -force
