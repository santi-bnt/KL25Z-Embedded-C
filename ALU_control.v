module ALU_control(
    input  [31:0] A,
    input  [31:0] B,
    input  [1:0]  ALUOp,
    input  [2:0]  funct3,
    input         op5,
    input         funct7_5,
    output [31:0] result,
    output        Zero
);

reg [2:0] Alucontrol;

always @(*) begin
    case (ALUOp)
        2'b00: Alucontrol = 3'b000; 
        2'b01: Alucontrol = 3'b001; 

        2'b10: begin
            case (funct3)
                3'b000: begin
                    if (op5 && funct7_5)
                        Alucontrol = 3'b001; 
                    else
                        Alucontrol = 3'b000; 
                end

                3'b010: Alucontrol = 3'b101; 
                3'b110: Alucontrol = 3'b011; 
                3'b111: Alucontrol = 3'b010; 

                default: Alucontrol = 3'b000;
            endcase
        end

        default: Alucontrol = 3'b000;
    endcase
end

ALU DECO (
    .A(A),
    .B(B),
    .Alucontrol(Alucontrol),
    .result(result),
    .Zero(Zero)
);

endmodule

