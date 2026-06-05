module EX_stage( // Etapa de ejecucion: usa la ALU, calcula branches y aplica forwarding
    input [31:0] RD1E,
    input [31:0] RD2E,
    input [31:0] PCE,
    input [31:0] ImmExtE,
    input [31:0] PCPlus4E,
    input [31:0] ResultW,
    input [31:0] ALUResultM,
    input [1:0] ForwardAE,
    input [1:0] ForwardBE,
    input AluSrcE,
    input [2:0] AluControlE,
    input BranchE,
    input JumpE,
    output [31:0] ALUResultE,
    output [31:0] WriteDataE,
    output [31:0] PCTargetE,
    output [31:0] PCPlus4OutE,
    output PCSrcE
);

reg [31:0] SrcAE;
reg [31:0] SrcBE_reg;
wire [31:0] SrcBE;
wire ZeroE;

always @(*) begin
    case (ForwardAE)
        2'b10: SrcAE = ALUResultM;
        2'b01: SrcAE = ResultW;
        default: SrcAE = RD1E;
    endcase

    case (ForwardBE)
        2'b10: SrcBE_reg = ALUResultM;
        2'b01: SrcBE_reg = ResultW;
        default: SrcBE_reg = RD2E;
    endcase
end

assign WriteDataE = SrcBE_reg;
assign SrcBE = AluSrcE ? ImmExtE : SrcBE_reg;
assign PCTargetE = PCE + ImmExtE;
assign PCPlus4OutE = PCPlus4E;
assign PCSrcE = (BranchE && ZeroE) || JumpE;

ALU alu(
    .A(SrcAE),
    .B(SrcBE),
    .Alucontrol(AluControlE),
    .result(ALUResultE),
    .Zero(ZeroE)
);

endmodule
