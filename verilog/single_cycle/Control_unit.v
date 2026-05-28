module control_unit( //conecta los decoders
    input [6:0] op,
    input [2:0] funct3,
    input funct7,
    input Zero,

    output [1:0] ResultSrc,
    output PCSrc,
    output MemWrite,
    output AluSrc,
    output [1:0] ImmSrc,
    output RegWrite,
    output [2:0] Alu_control
);

wire [1:0] Alu_op;

main_decoder main(
    .op(op),
    .Zero(Zero),
    .ResultSrc(ResultSrc),
    .PCSrc(PCSrc),
    .MemWrite(MemWrite),
    .AluSrc(AluSrc),
    .ImmSrc(ImmSrc),
    .RegWrite(RegWrite),
    .Alu_op(Alu_op)
);

ALU_decoder alu(
    .Alu_op(Alu_op),
    .funct3(funct3),
    .op(op[5]),
    .funct7(funct7),
    .Alu_Control(Alu_control)
);

endmodule