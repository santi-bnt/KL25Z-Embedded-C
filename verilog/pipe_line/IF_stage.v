module IF_stage( // Etapa de fetch: guarda el PC y lee la instruccion de memoria
    input clk,
    input rst,
    input StallF,
    input PCSrcE,
    input [31:0] PCTargetE,
    output reg [31:0] PCF,
    output [31:0] InstrF,
    output [31:0] PCPlus4F
);

wire [31:0] PCNextF;

assign PCPlus4F = PCF + 4;
assign PCNextF = PCSrcE ? PCTargetE : PCPlus4F;

always @(posedge clk or posedge rst) begin
    if (rst)
        PCF <= 0;
    else if (!StallF)
        PCF <= PCNextF;
end

Instruction_memory imem(
    .A(PCF),
    .RD(InstrF)
);

endmodule
