module WB_stage(
    input [1:0] ResultSrcW,
    input [31:0] ALUResultW,
    input [31:0] ReadDataW,
    input [31:0] PCPlus4W,
    output [31:0] ResultW
);

mux3 result_mux(
    .a(ALUResultW),
    .b(ReadDataW),
    .c(PCPlus4W),
    .select(ResultSrcW),
    .out(ResultW)
);

endmodule
