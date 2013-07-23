//*****************************************************************
// File:    vb_decoder_top.v
// Author:  Yang Sun (ysun@rice.edu)
// Birth:   $ 1/15/07
// Des:     viterbi decoder top level
//          Take 5 bit soft value is [-16 : +15]
//          K = 3. g0 =7, g1 = 5
//          K = 7. g0 = 133, g1 = 171
// History: $ 1/15/07, Init coding
//          $ 1/21/07, K = 7
//          $ 1/27/07, Change to LLR domain
//          $ 2/4/07, Support puncture 2/3, 3/4
//          $ 3/22/07, Remove puncture and iq buffer for WARP
//          $ 3/23/07, Fixed a naming problem for sysgen
//                     Can not use VHDL reserved key world
//          $ 4/20/07, K = 7
//          $ 4/22/07, Change quantilization to -4 ~ 4 9-level
//          $ 12/1/07: Modified for OFDM V7
//*****************************************************************
module vb_decoder_top (
        clk         ,   // I, clock 
		ce			,   // I, clock enable (ignored, but required for Sysgen blackbox)
        nrst        ,   // I, n reset
        packet_start,   // I, packet start pulse
        packet_end  ,   // I, packet end pulse
        zero_tail   ,   // I, the code is zero terminated
        vin         ,   // I, valid input
        llr_b1      ,   // I, 1st LLR
        llr_b0      ,   // I, 2nd LLR
        vout        ,   // O, valid output
        dout_in_byte,   // O, decoded output in byte
        done,            // O, decoding done
		trace_now
        ) ;
parameter           SW = 4 ;        // soft input precision
parameter           M = 7 ;         // Metric precision
parameter           L = 24 ;        // total trace depth //WLAN
parameter           R = 24 ;        // reliable trace
parameter           C = 0 ;        // unreliable trace
//parameter           L = 88 ;        // total trace depth
//parameter           R = 48 ;        // reliable trace
//parameter           C = 40 ;        // unreliable trace
parameter           LW = 7 ;        // L width
parameter           K = 7 ;         // constraint length
parameter           N = 64 ;        // number of states
parameter           TR = 128 ;      // trace buffer depth
parameter           TRW = 7 ;       // trace buffer address width
        
input               clk ;           // system clock
input				ce ;
input               nrst ;          // active low reset
input               packet_start ;  // start of packet pulse
input               zero_tail ;     // 1 = the code is terminated with 0, 0 = no termination
input               packet_end ;    // end of packet pulse
input               vin ;           // data valid input
input   [4 -1:0]   llr_b1 ;        // soft value for bit1
input   [4 -1:0]   llr_b0 ;        // soft value for bit0
input trace_now;

output              done ;
output              vout ;
output  [7:0]       dout_in_byte ;

//=============================================
//Internal signal
//=============================================
wire    [LW -1:0]           remain ;
wire                        dec_vout ;
wire    [R-1:0]             dec_dout ;
wire                        dec_done ;

//=============================================
// Main RTL code
//=============================================

//================================================================
// Viterbi decoder core logic
//================================================================
viterbi_core #(SW, M, R, C, L, LW, K, N, TR, TRW) viterbi_core (
        .clk            (clk            ),  //IN 
        .nrst           (nrst           ),  //IN 
        .packet_start   (packet_start   ),  //IN 
        .packet_end     (packet_end     ),  //IN
        .zero_tail      (zero_tail      ),
        .dv_in          (vin            ),  //IN 
        .llr1           (llr_b1         ),  //IN[SW-1:0] 
        .llr0           (llr_b0         ),  //IN[SW-1:0]        
        .remain         (remain         ),  //OUT[LW -1:0]
        .done           (dec_done       ),  //OUT
        .dv_out         (dec_vout       ),  //OUT
        .dout           (dec_dout       ),   //OUT[R -1:0]
		.trace_now (trace_now)
        ) ;

//=============================================
// x to 8bit unpacking
//=============================================        
unpack_m2n #(R, 8, LW) unpack_Rto8 (
        .clk    (clk            ),  //IN 
        .nrst   (nrst           ),  //IN 
        .start  (packet_start   ),  //IN 
        .din    (dec_dout       ),  //IN[R -1:0]   
        .vin    (dec_vout       ),  //IN 
        .last   (dec_done       ),  //IN
        .remain (remain         ),  //IN[LW -1:0]
        .dout   (dout_in_byte   ),  //OUT
        .vout   (vout           ),  //OUT
        .done   (done           )   //OUT
        ) ;
        
endmodule
