#ZCU102 PMOD BT2 J55 Connections

set_property PACKAGE_PIN A20      [get_ports "BLE_RTS"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_RTS"] ;

set_property PACKAGE_PIN B20      [get_ports "BLE_RX"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_RX"] ; 

set_property PACKAGE_PIN A22      [get_ports "BLE_TX"] ; 
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_TX"] ; 

set_property PACKAGE_PIN A21      [get_ports "BLE_CTS"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_CTS"] ;

set_property PACKAGE_PIN B21      [get_ports "BLE_GPIO"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_GPIO"] ;

set_property PACKAGE_PIN C21      [get_ports "BLE_RST"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_RST"] ;

set_property PACKAGE_PIN C22      [get_ports "BLE_MODE"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_MODE"] ;

set_property PACKAGE_PIN D21      [get_ports "BLE_RX_IND"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "BLE_RX_IND"] ;
