#ZCU102 PMOD BLE J55 Connections 

set_property PACKAGE_PIN A20      [get_ports "BLE_RTS"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_RTS"] ;

set_property PACKAGE_PIN B20      [get_ports "BLE_RX"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_RX"] ; 

set_property PACKAGE_PIN A22      [get_ports "BLE_TX"] ; 
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_TX"] ; 

set_property PACKAGE_PIN A21      [get_ports "BLE_CTS"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_CTS"] ;


set_property -dict { PACKAGE_PIN B21   IOSTANDARD LVCMOS33 } [get_ports { BLE_PINS_tri_io[0] }]; #BLE_GPIO
set_property -dict { PACKAGE_PIN C21   IOSTANDARD LVCMOS33 } [get_ports { BLE_PINS_tri_io[1] }]; #BLE_RST
set_property -dict { PACKAGE_PIN C22   IOSTANDARD LVCMOS33 } [get_ports { BLE_PINS_tri_io[2] }]; #BLE_Mode
set_property -dict { PACKAGE_PIN D21   IOSTANDARD LVCMOS33 } [get_ports { BLE_PINS_tri_io[3] }]; #BLE_Status


