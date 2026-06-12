module main(
    input clk,
    input rst
);

wire [31:0] pc;
wire [31:0] pc_next;
wire [23:0] instr_from_mem;
reg [23:0] IR;

wire [3:0] opcode;
wire [3:0] rd;
wire [3:0] rs1;
wire [3:0] rs2_r;
wire [3:0] rs2_s;
wire [3:0] rs2_b;
wire [3:0] A1;
wire [3:0] A2;
wire [3:0] A3;

wire [31:0] RD1;
wire [31:0] RD2;
reg [31:0] A;
reg [31:0] B;
wire [31:0] ImmExt;
wire [31:0] SrcA;
wire [31:0] SrcB;
wire [31:0] ALUResult;
wire Zero;
reg [31:0] ALUOut;
wire [31:0] ReadData;
reg [31:0] MDR;
wire [31:0] WriteData;

wire PCWrite;
wire IRWrite;
wire RegWrite;
wire MemRead;
wire MemWrite;
wire MDRWrite;
wire ALUOutWrite;
wire ALUSrcA;
wire [1:0] ALUSrcB;
wire [1:0] ALUOp;
wire MemToReg;
wire Branch;
wire Jump;
wire [1:0] PCSrc;
wire [2:0] state;

assign opcode = IR[23:20];
assign rd = IR[19:16];
assign rs1 = IR[15:12];
assign rs2_r = IR[11:8];
assign rs2_s = IR[19:16];
assign rs2_b = IR[15:12];

assign A1 = (opcode == 6) ? IR[19:16] : rs1;
assign A2 = (opcode == 5) ? rs2_s :
            (opcode == 6) ? rs2_b : rs2_r;
assign A3 = rd;

PC pc_reg(
    .clk(clk),
    .rst(rst),
    .PCWrite(PCWrite || (Branch && Zero)),
    .pc_next(pc_next),
    .pc(pc)
);

Instruction_memory imem(
    .A(pc),
    .RD(instr_from_mem)
);

Control_unit cu(
    .clk(clk),
    .rst(rst),
    .opcode(opcode),
    .Zero(Zero),
    .PCWrite(PCWrite),
    .IRWrite(IRWrite),
    .RegWrite(RegWrite),
    .MemRead(MemRead),
    .MemWrite(MemWrite),
    .MDRWrite(MDRWrite),
    .ALUOutWrite(ALUOutWrite),
    .ALUSrcA(ALUSrcA),
    .ALUSrcB(ALUSrcB),
    .ALUOp(ALUOp),
    .MemToReg(MemToReg),
    .Branch(Branch),
    .Jump(Jump),
    .PCSrc(PCSrc),
    .state(state)
);

RF register_file(
    .clk(clk),
    .WE3(RegWrite),
    .A1(A1),
    .A2(A2),
    .A3(A3),
    .WD3(WriteData),
    .RD1(RD1),
    .RD2(RD2)
);

Extend ext(
    .instr(IR),
    .ImmExt(ImmExt)
);

assign SrcA = (ALUSrcA == 0) ? pc : A;
assign SrcB = (ALUSrcB == 0) ? B :
              (ALUSrcB == 1) ? ImmExt : 32'd1;

ALU alu(
    .A(SrcA),
    .B(SrcB),
    .AluControl(ALUOp),
    .Result(ALUResult),
    .Zero(Zero)
);

Data_mem dmem(
    .clk(clk),
    .WE(MemWrite),
    .A(ALUOut),
    .WD(B),
    .RD(ReadData)
);

assign WriteData = (MemToReg == 1) ? MDR : ALUOut;

assign pc_next = (PCSrc == 0) ? ALUResult :
                 (PCSrc == 1) ? pc + ImmExt :
                 (PCSrc == 2) ? pc + ImmExt : ALUResult;

always @(posedge clk or posedge rst)
begin
    if(rst)
    begin
        IR <= 0;
        A <= 0;
        B <= 0;
        ALUOut <= 0;
        MDR <= 0;
    end
    else
    begin
        if(IRWrite)
            IR <= instr_from_mem;

        // En decode se guardan los registros para usarlos en otros ciclos
        if(state == 1)
        begin
            A <= RD1;
            B <= RD2;
        end

        if(ALUOutWrite)
            ALUOut <= ALUResult;

        if(MDRWrite)
            MDR <= ReadData;
    end
end

endmodule
