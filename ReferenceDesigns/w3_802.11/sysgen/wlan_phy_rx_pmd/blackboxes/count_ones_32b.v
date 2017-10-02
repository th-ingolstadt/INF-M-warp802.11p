module count_ones_32b (
	input clk,
	input ce,
	input [31:0] x,
	output reg [5:0] num_ones = 0
);

	reg [31:0] x_d = 0;

	always @(posedge clk)
	begin
		x_d <= x;
	end

	wire [2:0] cnt0, cnt1, cnt2, cnt3, cnt4;
	
	count_ones_6b c0(.x(x_d[ 5: 0]), .num_ones(cnt0));
	count_ones_6b c1(.x(x_d[11: 6]), .num_ones(cnt1));
	count_ones_6b c2(.x(x_d[17:12]), .num_ones(cnt2));
	count_ones_6b c3(.x(x_d[23:18]), .num_ones(cnt3));
	count_ones_6b c4(.x(x_d[29:24]), .num_ones(cnt4));

	always @(posedge clk)
	begin
		num_ones <= cnt0 + cnt1 + cnt2 + cnt3 + cnt4 + x_d[30] + x_d[31];
	end

endmodule

module count_ones_6b (
	input [5:0] x,
	output reg [2:0] num_ones = 0
);

	 always @* begin
		case (x)
		6'h00: num_ones = 0;
		6'h01: num_ones = 1;		6'h02: num_ones = 1;		6'h03: num_ones = 2;
		6'h04: num_ones = 1;		6'h05: num_ones = 2;		6'h06: num_ones = 2;		6'h07: num_ones = 3;
		6'h08: num_ones = 1;		6'h09: num_ones = 2;		6'h0A: num_ones = 2;		6'h0B: num_ones = 3;
		6'h0C: num_ones = 2;		6'h0D: num_ones = 3;		6'h0E: num_ones = 3;		6'h0F: num_ones = 4;
		6'h10: num_ones = 1;		6'h11: num_ones = 2;		6'h12: num_ones = 2;		6'h13: num_ones = 3;
		6'h14: num_ones = 2;		6'h15: num_ones = 3;		6'h16: num_ones = 3;		6'h17: num_ones = 4;
		6'h18: num_ones = 2;		6'h19: num_ones = 3;		6'h1A: num_ones = 3;		6'h1B: num_ones = 4;
		6'h1C: num_ones = 3;		6'h1D: num_ones = 4;		6'h1E: num_ones = 4;		6'h1F: num_ones = 5;
		6'h20: num_ones = 1;		6'h21: num_ones = 2;		6'h22: num_ones = 2;		6'h23: num_ones = 3;
		6'h24: num_ones = 2;		6'h25: num_ones = 3;		6'h26: num_ones = 3;		6'h27: num_ones = 4;
		6'h28: num_ones = 2;		6'h29: num_ones = 3;		6'h2A: num_ones = 3;		6'h2B: num_ones = 4;
		6'h2C: num_ones = 3;		6'h2D: num_ones = 4;		6'h2E: num_ones = 4;		6'h2F: num_ones = 5;
		6'h30: num_ones = 2;		6'h31: num_ones = 3;		6'h32: num_ones = 3;		6'h33: num_ones = 4;
		6'h34: num_ones = 3;		6'h35: num_ones = 4;		6'h36: num_ones = 4;		6'h37: num_ones = 5;
		6'h38: num_ones = 3;		6'h39: num_ones = 4;		6'h3A: num_ones = 4;		6'h3B: num_ones = 5;
		6'h3C: num_ones = 4;		6'h3D: num_ones = 5;		6'h3E: num_ones = 5;		6'h3F: num_ones = 6;
		default: num_ones = 0;
		endcase
	end

endmodule