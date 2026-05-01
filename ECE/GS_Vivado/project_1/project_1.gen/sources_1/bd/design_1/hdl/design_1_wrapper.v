//Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
//Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2025.1 (win64) Build 6140274 Thu May 22 00:12:29 MDT 2025
//Date        : Sun Mar  1 20:51:49 2026
//Host        : SDP2635 running 64-bit major release  (build 9200)
//Command     : generate_target design_1_wrapper.bd
//Design      : design_1_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module design_1_wrapper
   (BLE_CTS,
    BLE_GPIO,
    BLE_MODE,
    BLE_RST,
    BLE_RTS,
    BLE_RX,
    BLE_STATUS,
    BLE_TX);
  output BLE_CTS;
  input [0:0]BLE_GPIO;
  input [0:0]BLE_MODE;
  output [0:0]BLE_RST;
  input BLE_RTS;
  output BLE_RX;
  input [0:0]BLE_STATUS;
  input BLE_TX;

  wire BLE_CTS;
  wire [0:0]BLE_GPIO;
  wire [0:0]BLE_MODE;
  wire [0:0]BLE_RST;
  wire BLE_RTS;
  wire BLE_RX;
  wire [0:0]BLE_STATUS;
  wire BLE_TX;

  design_1 design_1_i
       (.BLE_CTS(BLE_CTS),
        .BLE_GPIO(BLE_GPIO),
        .BLE_MODE(BLE_MODE),
        .BLE_RST(BLE_RST),
        .BLE_RTS(BLE_RTS),
        .BLE_RX(BLE_RX),
        .BLE_STATUS(BLE_STATUS),
        .BLE_TX(BLE_TX));
endmodule
