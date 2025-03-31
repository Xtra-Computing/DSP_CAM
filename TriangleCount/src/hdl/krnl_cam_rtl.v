///////////////////////////////////////////////////////////////////////////////
// Description: This is a wrapper of module "krnl_cam_rtl_int"
///////////////////////////////////////////////////////////////////////////////

// default_nettype of none prevents implicit wire declaration.
`default_nettype none
`timescale 1 ns / 1 ps 

module krnl_cam_rtl #( 
  parameter integer  C_DATA_WIDTH = 520
)
(
  // System signals
  input  wire  ap_clk,
  input  wire  ap_rst_n,
  // AXI4 master interface 
  input  wire [C_DATA_WIDTH-1:0] p0_TDATA, // input data 
  input  wire        p0_TVALID,
  output wire        p0_TREADY,
  output wire [C_DATA_WIDTH-1:0] p1_TDATA, // output data
  output wire        p1_TVALID,
  input  wire        p1_TREADY
);

krnl_cam_rtl_int #(
  .C_DATA_WIDTH                ( C_DATA_WIDTH )
)
inst_krnl_cam_rtl_int (
  .ap_clk                 ( ap_clk ),
  .ap_rst_n               ( ap_rst_n ),
  .p0_TDATA               ( p0_TDATA ),
  .p0_TVALID              ( p0_TVALID ),
  .p0_TREADY              ( p0_TREADY ),
  .p1_TDATA               ( p1_TDATA ),
  .p1_TVALID              ( p1_TVALID ),
  .p1_TREADY              ( p1_TREADY )
);
endmodule : krnl_cam_rtl

`default_nettype wire
