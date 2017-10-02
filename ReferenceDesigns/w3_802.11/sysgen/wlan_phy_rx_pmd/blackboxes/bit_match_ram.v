module bit_match_ram (
  clk,
  ce,

  bit_in,
  bit_in_wr_en,
  
  addr_a,
  addr_b,
  
  dout_a,
  dout_b
);

//Sysgen blackboxes must have clk and ce ports
// VHDL wrapper uses std_logic for these signals
input clk;
input ce;

// Sysgen instantiates this module in a VHDL wrapper. The wrapper uses std_logic_vector(0:0) signals
//  to connect to this module's ports. It's *very* important the scalar I/O below have dimensions [0:0].
//  Without these XST bizarrely decides no connection is made and optimizes out the RAM block.
// Using Sysgen's "port.useHDLVector(false)" in the config.m would probably achieve the same thing.

input  [0:0] bit_in;
input  [0:0] bit_in_wr_en;

input [14:0] addr_a;
input [14:0] addr_b;

output [31:0] dout_a;
output [31:0] dout_b;

(* box_type = "black_box" *)
bit_match_ram_blkmemgen ram_inst (
	.clka(clk),
	.clkb(clk),
	
	.dina(bit_in),
	.dinb(1'b0),

	.wea(bit_in_wr_en),
	.web(1'b0),

	.addra(addr_a),
	.addrb(addr_b),
	
	.douta(dout_a),
	.doutb(dout_b)
);

endmodule


module bit_match_ram_blkmemgen(
  clka,
  wea,
  addra,
  dina,
  douta,
  clkb,
  web,
  addrb,
  dinb,
  doutb
);

input clka;
input [0 : 0] wea;
input [14 : 0] addra;
input [0 : 0] dina;
output [31 : 0] douta;
input clkb;
input [0 : 0] web;
input [14 : 0] addrb;
input [0 : 0] dinb;
output [31 : 0] doutb;

// synthesis translate_off

  BLK_MEM_GEN_V7_3 #(
    .C_ADDRA_WIDTH(15),
    .C_ADDRB_WIDTH(15),
    .C_ALGORITHM(1),
    .C_AXI_ID_WIDTH(4),
    .C_AXI_SLAVE_TYPE(0),
    .C_AXI_TYPE(1),
    .C_BYTE_SIZE(9),
    .C_COMMON_CLK(1),
    .C_DEFAULT_DATA("0"),
    .C_DISABLE_WARN_BHV_COLL(0),
    .C_DISABLE_WARN_BHV_RANGE(0),
    .C_ENABLE_32BIT_ADDRESS(0),
    .C_FAMILY("virtex6"),
    .C_HAS_AXI_ID(0),
    .C_HAS_ENA(0),
    .C_HAS_ENB(0),
    .C_HAS_INJECTERR(0),
    .C_HAS_MEM_OUTPUT_REGS_A(0),
    .C_HAS_MEM_OUTPUT_REGS_B(0),
    .C_HAS_MUX_OUTPUT_REGS_A(0),
    .C_HAS_MUX_OUTPUT_REGS_B(0),
    .C_HAS_REGCEA(0),
    .C_HAS_REGCEB(0),
    .C_HAS_RSTA(0),
    .C_HAS_RSTB(0),
    .C_HAS_SOFTECC_INPUT_REGS_A(0),
    .C_HAS_SOFTECC_OUTPUT_REGS_B(0),
    .C_INIT_FILE("BlankString"),
    .C_INIT_FILE_NAME("no_coe_file_loaded"),
    .C_INITA_VAL("0"),
    .C_INITB_VAL("0"),
    .C_INTERFACE_TYPE(0),
    .C_LOAD_INIT_FILE(0),
    .C_MEM_TYPE(2),
    .C_MUX_PIPELINE_STAGES(0),
    .C_PRIM_TYPE(1),
    .C_READ_DEPTH_A(1024),
    .C_READ_DEPTH_B(1024),
    .C_READ_WIDTH_A(32),
    .C_READ_WIDTH_B(32),
    .C_RST_PRIORITY_A("CE"),
    .C_RST_PRIORITY_B("CE"),
    .C_RST_TYPE("SYNC"),
    .C_RSTRAM_A(0),
    .C_RSTRAM_B(0),
    .C_SIM_COLLISION_CHECK("ALL"),
    .C_USE_BRAM_BLOCK(0),
    .C_USE_BYTE_WEA(0),
    .C_USE_BYTE_WEB(0),
    .C_USE_DEFAULT_DATA(0),
    .C_USE_ECC(0),
    .C_USE_SOFTECC(0),
    .C_WEA_WIDTH(1),
    .C_WEB_WIDTH(1),
    .C_WRITE_DEPTH_A(32768),
    .C_WRITE_DEPTH_B(32768),
    .C_WRITE_MODE_A("WRITE_FIRST"),
    .C_WRITE_MODE_B("WRITE_FIRST"),
    .C_WRITE_WIDTH_A(1),
    .C_WRITE_WIDTH_B(1),
    .C_XDEVICEFAMILY("virtex6")
  )
  inst (
    .CLKA(clka),
    .WEA(wea),
    .ADDRA(addra),
    .DINA(dina),
    .DOUTA(douta),
    .CLKB(clkb),
    .WEB(web),
    .ADDRB(addrb),
    .DINB(dinb),
    .DOUTB(doutb),
    .RSTA(),
    .ENA(),
    .REGCEA(),
    .RSTB(),
    .ENB(),
    .REGCEB(),
    .INJECTSBITERR(),
    .INJECTDBITERR(),
    .SBITERR(),
    .DBITERR(),
    .RDADDRECC(),
    .S_ACLK(),
    .S_ARESETN(),
    .S_AXI_AWID(),
    .S_AXI_AWADDR(),
    .S_AXI_AWLEN(),
    .S_AXI_AWSIZE(),
    .S_AXI_AWBURST(),
    .S_AXI_AWVALID(),
    .S_AXI_AWREADY(),
    .S_AXI_WDATA(),
    .S_AXI_WSTRB(),
    .S_AXI_WLAST(),
    .S_AXI_WVALID(),
    .S_AXI_WREADY(),
    .S_AXI_BID(),
    .S_AXI_BRESP(),
    .S_AXI_BVALID(),
    .S_AXI_BREADY(),
    .S_AXI_ARID(),
    .S_AXI_ARADDR(),
    .S_AXI_ARLEN(),
    .S_AXI_ARSIZE(),
    .S_AXI_ARBURST(),
    .S_AXI_ARVALID(),
    .S_AXI_ARREADY(),
    .S_AXI_RID(),
    .S_AXI_RDATA(),
    .S_AXI_RRESP(),
    .S_AXI_RLAST(),
    .S_AXI_RVALID(),
    .S_AXI_RREADY(),
    .S_AXI_INJECTSBITERR(),
    .S_AXI_INJECTDBITERR(),
    .S_AXI_SBITERR(),
    .S_AXI_DBITERR(),
    .S_AXI_RDADDRECC()
  );

// synthesis translate_on
endmodule
