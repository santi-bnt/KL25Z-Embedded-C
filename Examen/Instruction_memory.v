module Instruction_memory( // Memoria de instrucciones de 24 bits
    input [31:0] A,
    output [23:0] RD
);

reg [23:0] instr_mem [0:255];
integer i;

initial
begin
    for(i = 0; i < 256; i = i + 1)
        instr_mem[i] = 24'h000000;

    $readmemh("instrMem.hex", instr_mem);
end

assign RD = instr_mem[A[7:0]];

endmodule
