set_property SRC_FILE_INFO {cfile:C:/Users/ist21005/Documents/GS_Vivado/project_1/project_1.srcs/constrs_1/new/PMOD_ESP32.xdc rfile:../../../project_1.srcs/constrs_1/new/PMOD_ESP32.xdc id:1} [current_design]
set_property SRC_FILE_INFO {cfile:C:/Users/ist21005/Documents/GS_Vivado/project_1/project_1.srcs/constrs_1/new/PMOD_BLE.xdc rfile:../../../project_1.srcs/constrs_1/new/PMOD_BLE.xdc id:2} [current_design]
set_property src_info {type:XDC file:1 line:6 export:INPUT save:INPUT read:READ} [current_design]
set_property PACKAGE_PIN E20      [get_ports "ESP_RX"] ;
set_property src_info {type:XDC file:1 line:9 export:INPUT save:INPUT read:READ} [current_design]
set_property PACKAGE_PIN D22      [get_ports "ESP_TX"] ;
set_property src_info {type:XDC file:1 line:15 export:INPUT save:INPUT read:READ} [current_design]
set_property -dict { PACKAGE_PIN F20   IOSTANDARD LVCMOS33 } [get_ports { ESP_PINS_tri_io[0] }]; #ESP_GPIO_1
set_property src_info {type:XDC file:1 line:16 export:INPUT save:INPUT read:READ} [current_design]
set_property -dict { PACKAGE_PIN G20   IOSTANDARD LVCMOS33 } [get_ports { ESP_PINS_tri_io[1] }]; #ESP_RST
set_property src_info {type:XDC file:1 line:17 export:INPUT save:INPUT read:READ} [current_design]
set_property -dict { PACKAGE_PIN J20   IOSTANDARD LVCMOS33 } [get_ports { ESP_PINS_tri_io[2] }]; #ESP_Mode
set_property src_info {type:XDC file:1 line:18 export:INPUT save:INPUT read:READ} [current_design]
set_property -dict { PACKAGE_PIN J19   IOSTANDARD LVCMOS33 } [get_ports { ESP_PINS_tri_io[3] }]; #ESP_GPIO_2
set_property src_info {type:XDC file:2 line:3 export:INPUT save:INPUT read:READ} [current_design]
set_property PACKAGE_PIN A20      [get_ports "BLE_RTS"] ;
set_property src_info {type:XDC file:2 line:6 export:INPUT save:INPUT read:READ} [current_design]
set_property PACKAGE_PIN B20      [get_ports "BLE_RX"] ;
set_property src_info {type:XDC file:2 line:9 export:INPUT save:INPUT read:READ} [current_design]
set_property PACKAGE_PIN A22      [get_ports "BLE_TX"] ;
set_property src_info {type:XDC file:2 line:12 export:INPUT save:INPUT read:READ} [current_design]
set_property PACKAGE_PIN A21      [get_ports "BLE_CTS"] ;
set_property src_info {type:XDC file:2 line:16 export:INPUT save:INPUT read:READ} [current_design]
set_property -dict { PACKAGE_PIN B21   IOSTANDARD LVCMOS33 } [get_ports { BLE_PINS_tri_io[0] }]; #BLE_GPIO
set_property src_info {type:XDC file:2 line:17 export:INPUT save:INPUT read:READ} [current_design]
set_property -dict { PACKAGE_PIN C21   IOSTANDARD LVCMOS33 } [get_ports { BLE_PINS_tri_io[1] }]; #BLE_RST
set_property src_info {type:XDC file:2 line:18 export:INPUT save:INPUT read:READ} [current_design]
set_property -dict { PACKAGE_PIN C22   IOSTANDARD LVCMOS33 } [get_ports { BLE_PINS_tri_io[2] }]; #BLE_Mode
set_property src_info {type:XDC file:2 line:19 export:INPUT save:INPUT read:READ} [current_design]
set_property -dict { PACKAGE_PIN D21   IOSTANDARD LVCMOS33 } [get_ports { BLE_PINS_tri_io[3] }]; #BLE_Status
