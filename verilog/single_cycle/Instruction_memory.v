module Instruction_memory(       //Guarda las instrucciones y da una para que se ejecute
    input [31:0] A,
    output [31:0] RD
);

reg [31:0] memory [0:255];

// Lee la memoria 
initial begin
   $readmemh("test_program.mem", memory);
end

assign RD = memory[A[31:2]];

endmodule