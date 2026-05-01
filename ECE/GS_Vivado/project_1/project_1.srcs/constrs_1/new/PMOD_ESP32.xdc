#ZCU102 PMOD BLE J55 Connections 

#set_property PACKAGE_PIN D20      [get_ports "ESP_RTS"] ;
#set_property IOSTANDARD  LVCMOS33 [get_ports "ESP_RTS"] ;

set_property PACKAGE_PIN E20      [get_ports "ESP_RX"] ;
set_property IOSTANDARD  LVCMOS33 [get_ports "ESP_RX"] ; 

set_property PACKAGE_PIN D22      [get_ports "ESP_TX"] ; 
set_property IOSTANDARD  LVCMOS33 [get_ports "ESP_TX"] ; 

#set_property PACKAGE_PIN E22      [get_ports "ESP_CTS"] ;
#set_property IOSTANDARD  LVCMOS33 [get_ports "ESP_CTS"] ;

set_property -dict { PACKAGE_PIN F20   IOSTANDARD LVCMOS33 } [get_ports { ESP_PINS_tri_io[0] }]; #ESP_GPIO_1
set_property -dict { PACKAGE_PIN G20   IOSTANDARD LVCMOS33 } [get_ports { ESP_PINS_tri_io[1] }]; #ESP_RST
set_property -dict { PACKAGE_PIN J20   IOSTANDARD LVCMOS33 } [get_ports { ESP_PINS_tri_io[2] }]; #ESP_Mode
set_property -dict { PACKAGE_PIN J19   IOSTANDARD LVCMOS33 } [get_ports { ESP_PINS_tri_io[3] }]; #ESP_GPIO_2