module main(
    input clk,rst
);

wire Equal;
wire [31:0] pc_target;
wire [31:0] pc;
wire [31:0] pc_next;
wire [31:0] pc_plus4;
wire [31:0] instr;

wire [31:0] RD1;
wire [31:0] RD2;
wire [31:0] ImmExt;
wire [31:0] SrcB;
wire [31:0] ALUResult;
wire [31:0] ReadData;
wire [31:0] Result;

wire Zero;
wire PCSrc;
wire MemWrite;
wire AluSrc;
wire RegWrite;

wire [1:0] ResultSrc;
wire [1:0] ImmSrc;
wire [2:0] Alu_control;

PC pc_module(
    .clk(clk),
    .rst(rst),
    .pc_next(pc_next),
    .pc(pc)
);

adder pc_adder(
    .a(pc),
    .b(32'd4),
    .result(pc_plus4)
);

Instruction_memory imem(
    .clk(clk),
    .A(pc),
    .RD(instr)
);

control_unit cu(
    .op(instr[6:0]),
    .funct3(instr[14:12]),
    .funct7(instr[30]),
    .Zero(Equal),
    .ResultSrc(ResultSrc),
    .PCSrc(PCSrc),
    .MemWrite(MemWrite),
    .AluSrc(AluSrc),
    .ImmSrc(ImmSrc),
    .RegWrite(RegWrite),
    .Alu_control(Alu_control)
);

RF register_f(
    .clk(clk),
    .WE3(RegWrite),
    .A1(instr[19:15]),
    .A2(instr[24:20]),
    .A3(instr[11:7]),
    .WD3(Result),
    .RD1(RD1),
    .RD2(RD2)
);

Extend ext(
    .instr(instr[31:7]),
    .ImmSrc(ImmSrc),
    .ImmExt(ImmExt)
);

// MUX para entrada B de la ALU
mux mux_alu_src(
    .a(RD2),
    .b(ImmExt),
    .select(AluSrc),
    .out(SrcB)
);

ALU alu(
    .A(RD1),
    .B(SrcB),
    .Alucontrol(Alu_control),
    .result(ALUResult),
    .Zero(Equal)
);

Data_mem dmem(
    .clk(clk),
    .WE(MemWrite),
    .A(ALUResult),
    .WD(RD2),
    .RD(ReadData)
);

// MUX para elegir resultado final
mux3 mux_result(
    .a(ALUResult),
    .b(ReadData),
    .c(pc_plus4),
    .select(ResultSrc),
    .out(Result)
);

adder pc_target_adder(
    .a(pc),
    .b(ImmExt),
    .result(pc_target)
);

mux mux_pc(
    .a(pc_plus4),
    .b(pc_target),
    .select(PCSrc),
    .out(pc_next)
);

branch_comparator bc(
    .A(RD1),
    .B(RD2),
    .Equal(Equal)
);

endmodule