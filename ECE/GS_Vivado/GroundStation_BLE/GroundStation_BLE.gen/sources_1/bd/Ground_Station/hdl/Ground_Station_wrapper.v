//Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
//Copyright 2022-2025 Advanced Micro Devices, Inc. All Rights Reserved.
//--------------------------------------------------------------------------------
//Tool Version: Vivado v.2025.1 (win64) Build 6140274 Thu May 22 00:12:29 MDT 2025
//Date        : Fri Feb 27 17:17:06 2026
//Host        : SDP2635 running 64-bit major release  (build 9200)
//Command     : generate_target Ground_Station_wrapper.bd
//Design      : Ground_Station_wrapper
//Purpose     : IP block netlist
//--------------------------------------------------------------------------------
`timescale 1 ps / 1 ps

module Ground_Station_wrapper
   (BLE_RST,
    BLE_RX,
    BLE_TX);
  output [0:0]BLE_RST;
  output BLE_RX;
  input BLE_TX;

  wire [0:0]BLE_RST;
  wire BLE_RX;
  wire BLE_TX;

  Ground_Station Ground_Station_i
       (.BLE_RST(BLE_RST),
        .BLE_RX(BLE_RX),
        .BLE_TX(BLE_TX));
endmodule
