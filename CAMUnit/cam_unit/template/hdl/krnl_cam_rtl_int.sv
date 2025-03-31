`default_nettype none
`timescale 1 ns / 1 ps 
(* DONT_TOUCH = "FALSE" *)

`define DATA_WIDTH CUSTOMIZED_BUS_WIDTH
`define CAM_BLOCK_SIZE CUSTOMIZED_CAM_SIZE

module krnl_cam_rtl_int #( 
  parameter integer  CAM_SIZE = `CAM_BLOCK_SIZE,
  parameter integer  C_DATA_WIDTH = `DATA_WIDTH
)
(
  // System signals
  input  wire  ap_clk,
  input  wire  ap_rst_n,
  // AXI4 master interface 
  input  wire [C_DATA_WIDTH-1:0] p0_TDATA,
  input  wire        p0_TVALID,
  output wire        p0_TREADY,
  output wire [C_DATA_WIDTH-1:0] p1_TDATA,
  output wire        p1_TVALID,
  input  wire        p1_TREADY
);

///////////////////////////////////////////////////////////////////////////////
// RTL Logic 
localparam integer                LP_FIFO_DEPTH = 32;
logic                             cam_tvalid;
logic                             cam_tready_n; 
logic                             wr_fifo_prog_full;
logic                             wr_fifo_tready; 
logic                             wr_fifo_tvalid_n;
logic [C_DATA_WIDTH-1:0]          wr_fifo_tdata;
logic [C_DATA_WIDTH-1:0]          cam_tdata;
logic                             areset;
logic                             p0_tready_reg;

always @(posedge ap_clk) begin 
  areset <= ~ap_rst_n; 
end

// Combinatorial Adder
krnl_cam_rtl_dsp #( 
  .C_DATA_WIDTH   ( C_DATA_WIDTH           ) ,
  .CAM_SIZE       ( CAM_SIZE                )
)
inst_dsp ( 
  .aclk     ( ap_clk            ) ,
  .areset   ( areset            ) ,
  .s_tvalid ( p0_TVALID & p0_TREADY ) ,
  .s_tdata  ( p0_TDATA          ) ,
  .m_tvalid ( cam_tvalid      ) ,
  .m_tdata  ( cam_tdata       )
);

// xpm_fifo_sync: Synchronous FIFO
// Xilinx Parameterized Macro, Version 2016.4
xpm_fifo_sync # (
  .FIFO_MEMORY_TYPE          ("auto"),           //string; "auto", "block", "distributed", or "ultra";
  .ECC_MODE                  ("no_ecc"),         //string; "no_ecc" or "en_ecc";
  .FIFO_WRITE_DEPTH          (LP_FIFO_DEPTH),   //positive integer
  .WRITE_DATA_WIDTH          (C_DATA_WIDTH),               //positive integer
  .WR_DATA_COUNT_WIDTH       ($clog2(LP_FIFO_DEPTH+1)),               //positive integer, Not used
  .PROG_FULL_THRESH          (LP_FIFO_DEPTH-5),               //positive integer, 3 pipeline stages, 
  .FULL_RESET_VALUE          (1),                //positive integer; 0 or 1
  .READ_MODE                 ("fwft"),            //string; "std" or "fwft";
  .FIFO_READ_LATENCY         (1),                //positive integer;
  .READ_DATA_WIDTH           (C_DATA_WIDTH),               //positive integer
  .RD_DATA_COUNT_WIDTH       ($clog2(LP_FIFO_DEPTH+1)),               //positive integer, not used
  .PROG_EMPTY_THRESH         (10),               //positive integer, not used 
  .DOUT_RESET_VALUE          ("0"),              //string, don't care
  .WAKEUP_TIME               (0)                 //positive integer; 0 or 2;

) inst_wr_xpm_fifo_sync (
  .sleep         ( 1'b0             ) ,
  .rst           ( areset           ) ,
  .wr_clk        ( ap_clk           ) ,
  .wr_en         ( cam_tvalid     ) ,
  .din           ( cam_tdata      ) ,
  .full          ( cam_tready_n   ) ,
  .prog_full     ( wr_fifo_prog_full) ,
  .wr_data_count (                  ) ,
  .overflow      (                  ) ,
  .wr_rst_busy   (                  ) ,
  .rd_en         ( wr_fifo_tready   ) ,
  .dout          ( wr_fifo_tdata    ) ,
  .empty         ( wr_fifo_tvalid_n ) ,
  .prog_empty    (                  ) ,
  .rd_data_count (                  ) ,
  .underflow     (                  ) ,
  .rd_rst_busy   (                  ) ,
  .injectsbiterr ( 1'b0             ) ,
  .injectdbiterr ( 1'b0             ) ,
  .sbiterr       (                  ) ,
  .dbiterr       (                  ) 

);



always @(posedge ap_clk) begin 
  p0_tready_reg <= ~wr_fifo_prog_full /*& ~ap_idle*/;
end

assign p0_TREADY = p0_tready_reg;

assign p1_TVALID = ~wr_fifo_tvalid_n;
assign p1_TDATA  = wr_fifo_tdata; 
assign wr_fifo_tready = p1_TREADY;

endmodule : krnl_cam_rtl_int

`default_nettype wire
