module Instruction_memory(          //leer memoria
	input clk,
	input [31:0] A,
	
	output reg [31:0] RD
);

	reg [31:0] instr_mem [0:2]; //0:10_000
	
	initial begin
		$readmemh("instrMem.hex",instr_mem);
		$display("instr_mem[0] = %h", instr_mem[0]);
		$display("instr_mem[1] = %h", instr_mem[1]);
		$display("instr_mem[2] = %h", instr_mem[2]);
	end
	
	always @(posedge clk)
		begin
		RD <= instr_mem[A[31:2]];
		end

endmodule
