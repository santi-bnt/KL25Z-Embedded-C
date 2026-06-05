module main(
    input clk,
    input rst
);

wire StallF, StallD, FlushD, FlushE;
wire [1:0] ForwardAE, ForwardBE;

wire [31:0] PCF, InstrF, PCPlus4F;
reg [31:0] InstrD, PCD, PCPlus4D;

wire [31:0] RD1D, RD2D, ImmExtD;
wire [4:0] Rs1D, Rs2D, RdD;
wire [1:0] ResultSrcD;
wire MemWriteD, AluSrcD, RegWriteD, BranchD, JumpD;
wire [2:0] AluControlD;

reg [31:0] RD1E, RD2E, PCE, ImmExtE, PCPlus4E;
reg [4:0] Rs1E, Rs2E, RdE;
reg [1:0] ResultSrcE;
reg MemWriteE, AluSrcE, RegWriteE, BranchE, JumpE;
reg [2:0] AluControlE;

wire [31:0] ALUResultE, WriteDataE, PCTargetE, PCPlus4OutE;
wire PCSrcE;

reg [31:0] ALUResultM, WriteDataM, PCPlus4M;
reg [4:0] RdM;
reg [1:0] ResultSrcM;
reg MemWriteM, RegWriteM;
wire [31:0] ReadDataM;

reg [31:0] ALUResultW, ReadDataW, PCPlus4W;
reg [4:0] RdW;
reg [1:0] ResultSrcW;
reg RegWriteW;
wire [31:0] ResultW;

IF_stage if_stage(
    .clk(clk),
    .rst(rst),
    .StallF(StallF),
    .PCSrcE(PCSrcE),
    .PCTargetE(PCTargetE),
    .PCF(PCF),
    .InstrF(InstrF),
    .PCPlus4F(PCPlus4F)
);

always @(posedge clk or posedge rst) begin
    if (rst || FlushD) 
    begin
        InstrD <= 0;
        PCD <= 0;
        PCPlus4D <= 0;
    end else if (!StallD) 
    begin
        InstrD <= InstrF;
        PCD <= PCF;
        PCPlus4D <= PCPlus4F;
    end
end

ID_stage id(
    .clk(clk),
    .InstrD(InstrD),
    .RegWriteW(RegWriteW),
    .RdW(RdW),
    .ResultW(ResultW),
    .RD1D(RD1D),
    .RD2D(RD2D),
    .ImmExtD(ImmExtD),
    .Rs1D(Rs1D),
    .Rs2D(Rs2D),
    .RdD(RdD),
    .ResultSrcD(ResultSrcD),
    .MemWriteD(MemWriteD),
    .AluSrcD(AluSrcD),
    .RegWriteD(RegWriteD),
    .AluControlD(AluControlD),
    .BranchD(BranchD),
    .JumpD(JumpD)
);

always @(posedge clk or posedge rst) begin
    if (rst || FlushE) begin
        RD1E <= 0;
        RD2E <= 0;
        PCE <= 0;
        ImmExtE <= 0;
        PCPlus4E <= 0;
        Rs1E <= 0;
        Rs2E <= 0;
        RdE <= 0;
        ResultSrcE <= 0;
        MemWriteE <= 0;
        AluSrcE <= 0;
        RegWriteE <= 0;
        BranchE <= 0;
        JumpE <= 0;
        AluControlE <= 0;
    end else begin
        RD1E <= RD1D;
        RD2E <= RD2D;
        PCE <= PCD;
        ImmExtE <= ImmExtD;
        PCPlus4E <= PCPlus4D;
        Rs1E <= Rs1D;
        Rs2E <= Rs2D;
        RdE <= RdD;
        ResultSrcE <= ResultSrcD;
        MemWriteE <= MemWriteD;
        AluSrcE <= AluSrcD;
        RegWriteE <= RegWriteD;
        BranchE <= BranchD;
        JumpE <= JumpD;
        AluControlE <= AluControlD;
    end
end

EX_stage ex(
    .RD1E(RD1E),
    .RD2E(RD2E),
    .PCE(PCE),
    .ImmExtE(ImmExtE),
    .PCPlus4E(PCPlus4E),
    .ResultW(ResultW),
    .ALUResultM(ALUResultM),
    .ForwardAE(ForwardAE),
    .ForwardBE(ForwardBE),
    .AluSrcE(AluSrcE),
    .AluControlE(AluControlE),
    .BranchE(BranchE),
    .JumpE(JumpE),
    .ALUResultE(ALUResultE),
    .WriteDataE(WriteDataE),
    .PCTargetE(PCTargetE),
    .PCPlus4OutE(PCPlus4OutE),
    .PCSrcE(PCSrcE)
);

always @(posedge clk or posedge rst) begin
    if (rst) begin
        ALUResultM <= 0;
        WriteDataM <= 0;
        PCPlus4M <= 0;
        RdM <= 0;
        ResultSrcM <= 0;
        MemWriteM <= 0;
        RegWriteM <= 0;
    end else begin
        ALUResultM <= ALUResultE;
        WriteDataM <= WriteDataE;
        PCPlus4M <= PCPlus4OutE;
        RdM <= RdE;
        ResultSrcM <= ResultSrcE;
        MemWriteM <= MemWriteE;
        RegWriteM <= RegWriteE;
    end
end

MEM_stage mem(
    .clk(clk),
    .MemWriteM(MemWriteM),
    .ALUResultM(ALUResultM),
    .WriteDataM(WriteDataM),
    .ReadDataM(ReadDataM)
);

always @(posedge clk or posedge rst) begin
    if (rst) begin
        ALUResultW <= 0;
        ReadDataW <= 0;
        PCPlus4W <= 0;
        RdW <= 0;
        ResultSrcW <= 0;
        RegWriteW <= 0;
    end else begin
        ALUResultW <= ALUResultM;
        ReadDataW <= ReadDataM;
        PCPlus4W <= PCPlus4M;
        RdW <= RdM;
        ResultSrcW <= ResultSrcM;
        RegWriteW <= RegWriteM;
    end
end

WB_stage wb(
    .ResultSrcW(ResultSrcW),
    .ALUResultW(ALUResultW),
    .ReadDataW(ReadDataW),
    .PCPlus4W(PCPlus4W),
    .ResultW(ResultW)
);

HazardUnit hazard(
    .Rs1D(Rs1D),
    .Rs2D(Rs2D),
    .Rs1E(Rs1E),
    .Rs2E(Rs2E),
    .RdE(RdE),
    .RdM(RdM),
    .RdW(RdW),
    .ResultSrcE(ResultSrcE),
    .RegWriteM(RegWriteM),
    .RegWriteW(RegWriteW),
    .PCSrcE(PCSrcE),
    .StallF(StallF),
    .StallD(StallD),
    .FlushD(FlushD),
    .FlushE(FlushE),
    .ForwardAE(ForwardAE),
    .ForwardBE(ForwardBE)
);


endmodule
