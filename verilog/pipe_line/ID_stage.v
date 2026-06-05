module ID_stage(
    input clk,
    input [31:0] InstrD,
    input RegWriteW,
    input [4:0] RdW,
    input [31:0] ResultW,
    output [31:0] RD1D,
    output [31:0] RD2D,
    output [31:0] ImmExtD,
    output [4:0] Rs1D,
    output [4:0] Rs2D,
    output [4:0] RdD,
    output [1:0] ResultSrcD,
    output MemWriteD,
    output AluSrcD,
    output RegWriteD,
    output [2:0] AluControlD,
    output BranchD,
    output JumpD
);

wire [1:0] ImmSrcD;
wire unused_PCSrcD;

assign Rs1D = InstrD[19:15];
assign Rs2D = InstrD[24:20];
assign RdD = InstrD[11:7];
assign BranchD = (InstrD[6:0] == 7'b1100011);
assign JumpD = (InstrD[6:0] == 7'b1101111);

control_unit cu(
    .op(InstrD[6:0]),
    .funct3(InstrD[14:12]),
    .funct7(InstrD[30]),
    .Zero(1'b0),
    .ResultSrc(ResultSrcD),
    .PCSrc(unused_PCSrcD),
    .MemWrite(MemWriteD),
    .AluSrc(AluSrcD),
    .ImmSrc(ImmSrcD),
    .RegWrite(RegWriteD),
    .Alu_control(AluControlD)
);

RF register_f(
    .clk(clk),
    .WE3(RegWriteW),
    .A1(Rs1D),
    .A2(Rs2D),
    .A3(RdW),
    .WD3(ResultW),
    .RD1(RD1D),
    .RD2(RD2D)
);

Extend ext(
    .instr(InstrD[31:7]),
    .ImmSrc(ImmSrcD),
    .ImmExt(ImmExtD)
);

endmodule
