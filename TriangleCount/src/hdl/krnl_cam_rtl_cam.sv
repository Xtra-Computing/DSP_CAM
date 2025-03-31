// We set register C as the storage register for the CAM.
// We set register [A:B] as the search register for the CAM.

`define IDLE 0
`define UPDATE_ALL 1
`define UPDATE_GROUP 2
`define UPDATE_ONE 3
`define SEARCH_ONE 4
`define SEARCH_MQ 5
`define SET_ROUTING_TABLE 6
`define RESET_ALL 7
`define UPDATE_DUPLICATE 8

`define END_OF_STREAM 4'hF

// `default_nettype none
(* DONT_TOUCH = "FALSE" *)
module krnl_cam_rtl_dsp #(
  parameter integer C_DATA_WIDTH   = 520, // Data width of both input and output data
  parameter integer CAM_SIZE = 128,
  parameter integer INDEX_WIDTH = $clog2(CAM_SIZE)
)
(
  input logic                                        aclk,
  input logic                                        areset,     
  input logic                                        s_tvalid,
  input logic [C_DATA_WIDTH-1:0]                     s_tdata,
  output logic                                       m_tvalid,
  output logic [C_DATA_WIDTH-1:0]                    m_tdata
);

timeunit 1ps; 
timeprecision 1ps; 

/////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////
logic m_tvalid_1;
logic [INDEX_WIDTH-1:0] write_index; 
logic [CAM_SIZE-1:0] resultSearchMQ;
logic resultOR;   // Single-bit result
logic [3:0] state;
logic [3:0] state_1, state_2;

assign resultOR = |resultSearchMQ;
assign state = s_tdata[518:515];
assign m_tdata = {state_2, 515'b0, resultOR};

always_ff @(posedge aclk) begin
  if (state == `RESET_ALL) begin
    write_index <= 0;
  end
  else if ((state == `UPDATE_DUPLICATE) && (s_tvalid)) begin
    write_index <= write_index + 16;
  end
end

always_ff @(posedge aclk) begin
  if (areset) begin
    m_tvalid_1 <= 0;
    state_1 <= 4'b0;
    state_2 <= 4'b0;
  end
  else begin
    m_tvalid_1 <= ((state == `SEARCH_MQ) || (state == `END_OF_STREAM)) ? s_tvalid : 0;
    m_tvalid <= m_tvalid_1;
    state_1 <= state;
    state_2 <= state_1;
  end
end

generate begin
  genvar i;
  for (i = 0; i < CAM_SIZE; i++) begin
  DSP48E2 #(
    // Feature Control Attributes: Data Path Selection
    .AMULTSEL("A"),                    // Selects A input to multiplier (A, AD)
    .A_INPUT("DIRECT"),                // Selects A input source, "DIRECT" (A port) or "CASCADE" (ACIN port)
    .BMULTSEL("B"),                    // Selects B input to multiplier (AD, B)
    .B_INPUT("DIRECT"),                // Selects B input source, "DIRECT" (B port) or "CASCADE" (BCIN port)
    .PREADDINSEL("A"),                 // Selects input to pre-adder (A, B)
    .RND(48'h000000000000),            // Rounding Constant
    .USE_MULT("NONE"),                 // Disable the multiplier, as multiplication is not needed
    .USE_SIMD("ONE48"),                // SIMD selection (FOUR12, ONE48, TWO24)
    .USE_WIDEXOR("FALSE"),              // Enable the Wide XOR function for 48-bit operation
    .XORSIMD("XOR24_48_96"),           // Enable full 48-bit XOR function
    // Pattern Detector Attributes: Pattern Detection Configuration
    .AUTORESET_PATDET("NO_RESET"),     // No reset for pattern detection
    .AUTORESET_PRIORITY("RESET"),      // Priority of AUTORESET vs. CEP (CEP, RESET)
    .MASK(48'h0),           // 48-bit mask value for pattern detect (1=ignore)
    .PATTERN(48'h000000000000),        // 48-bit pattern match for pattern detect
    .SEL_MASK("MASK"),                 // Select MASK value for pattern detection
    .SEL_PATTERN("PATTERN"),           // Select pattern value for pattern detection
    .USE_PATTERN_DETECT("PATDET"),  // Disable pattern detection
    // Programmable Inversion Attributes: Specifies built-in programmable inversion on specific pins
    .IS_ALUMODE_INVERTED(4'b0000),     // No inversion for ALUMODE
    .IS_CARRYIN_INVERTED(1'b0),        // No inversion for CARRYIN
    .IS_CLK_INVERTED(1'b0),            // No inversion for CLK
    .IS_INMODE_INVERTED(5'b00000),     // No inversion for INMODE
    .IS_OPMODE_INVERTED(9'b000000000), // No inversion for OPMODE
    .IS_RSTALLCARRYIN_INVERTED(1'b0),  // No inversion for RSTALLCARRYIN
    .IS_RSTALUMODE_INVERTED(1'b0),     // No inversion for RSTALUMODE
    .IS_RSTA_INVERTED(1'b0),           // No inversion for RSTA
    .IS_RSTB_INVERTED(1'b0),           // No inversion for RSTB
    .IS_RSTCTRL_INVERTED(1'b0),        // No inversion for RSTCTRL
    .IS_RSTC_INVERTED(1'b0),           // No inversion for RSTC
    .IS_RSTD_INVERTED(1'b0),           // No inversion for RSTD
    .IS_RSTINMODE_INVERTED(5'b00000),  // No inversion for RSTINMODE
    .IS_RSTM_INVERTED(1'b0),           // No inversion for RSTM
    .IS_RSTP_INVERTED(1'b0),           // No inversion for RSTP
    // Register Control Attributes: Pipeline Register Configuration
    .ACASCREG(1),                      // Number of pipeline stages between A/ACIN and ACOUT (0-2)
    .ADREG(1),                         // Pipeline stages for pre-adder (0-1)
    .ALUMODEREG(0),                    // Pipeline stages for ALUMODE (0-1)
    .AREG(1),                          // Pipeline stages for A (0-2)
    .BCASCREG(1),                      // Number of pipeline stages between B/BCIN and BCOUT (0-2)
    .BREG(1),                          // Pipeline stages for B (0-2)
    .CARRYINREG(0),                    // Disable carry-in register
    .CARRYINSELREG(0),                 // Disable carry-in select register
    .CREG(1),                          // Pipeline stages for C (0-1)
    .DREG(1),                          // Disable D input register
    .INMODEREG(0),                     // Disable INMODE register
    .MREG(0),                          // Disable multiplier register
    .OPMODEREG(0),                     // Enable OPMODE register for better control
    .PREG(1)                           // Disable P register for direct output
  )
  DSP48E2_inst (
    // Control outputs: Control Inputs/Status Bits
    .OVERFLOW(),             // Overflow status output (not used)
    .UNDERFLOW(),            // Underflow status output (not used)
    .PATTERNBDETECT(),       // Pattern B detect output (not used)
    .PATTERNDETECT(resultSearchMQ[i]),        // Pattern detect output
    // Data outputs: Data Ports
    .P(),                   // 48-bit output: Result of A:B XOR C
    // Data inputs: Data Ports
    .A({16'b0, s_tdata[31:18]}),                   // 30-bit input: A data
    .B(s_tdata[17:0]),                   // 18-bit input: B data
    .C((state == `RESET_ALL) ? 48'b0 : {16'b0, s_tdata[(i%16)*32+:32]}),                   // 48-bit input: C data
    // .C(data_in[i][47:0]),                   // 48-bit input: C data
    // Control inputs: Control Inputs/Status Bits
    .ALUMODE(4'b0100),       // Set ALUMODE to perform XOR operation, 4'b0100
    .OPMODE(9'b000110011),   // Set OPMODE to enable A:B XOR C operation, 9'b000110011
    .CLK(aclk),               // Clock signal
    .CEA1(1'b0),             // Clock enable for A input register
    // .CEA2(m_tready),             // Clock enable for A input register
    .CEA2(1'b1),
    .CEB1(1'b0),             // Clock enable for B input register
    // .CEB2(m_tready),             // Clock enable for B input register
    .CEB2(1'b1),
    .CEC(((state == `UPDATE_DUPLICATE) && (s_tvalid) && (write_index <= i) && (write_index+16 > i)) || (state == `RESET_ALL)),
    // .CEC(1'b1),             // Clock enable for C input register
    .CEP(1'b1),             // pipeline stall
    .CEALUMODE(1'b1),         // Clock enable for ALUMODE register
    .CECTRL(1'b1),           // Clock enable for control register 
    .CED(1'b0),             // Clock enable for D input register (not used)
    .CEAD(1'b0),            // Clock enable for AD input register (not used)
    .RSTA(1'b0),             // Reset for A input register
    .RSTB(1'b0),             // Reset for B input register
    // .RSTC(state == `RESET_ALL),             // Reset for C input register
    .RSTC(1'b0),             // Reset for C input register
    .RSTD(1'b0),             // Reset for D input register (not used)
    .RSTALLCARRYIN(1'b0),    // Reset for carry-in register (not used)
    .RSTALUMODE(1'b0),       // Reset for ALUMODE register
    .RSTCTRL(1'b0),          // Reset for control register
    .RSTP(1'b0)              // Reset for output register
  );
end
end
endgenerate

endmodule : krnl_cam_rtl_dsp
