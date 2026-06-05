module MEM_stage( // Etapa de memoria: lee o escribe datos usando la direccion de la ALU
    input clk,
    input MemWriteM,
    input [31:0] ALUResultM,
    input [31:0] WriteDataM,
    output [31:0] ReadDataM
);

Data_mem dmem(
    .clk(clk),
    .WE(MemWriteM),
    .A(ALUResultM),
    .WD(WriteDataM),
    .RD(ReadDataM)
);

endmodule
