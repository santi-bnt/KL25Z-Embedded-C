module Instruction_memory(
    input clk,
    input [31:0] A,
    output reg [31:0] RD
);

reg [31:0] instr_mem [0:255];

initial begin
    $readmemh("test_program.mem", instr_mem);

    $display("instr_mem[0] = %h", instr_mem[0]);
    $display("instr_mem[1] = %h", instr_mem[1]);
    $display("instr_mem[2] = %h", instr_mem[2]);
end

always @(posedge clk)
begin
    RD <= instr_mem[A[31:2]];
end

endmodule