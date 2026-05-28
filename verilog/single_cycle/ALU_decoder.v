module ALU_decoder(                     // Decide que operacion hara la ALU usando aluop, funct3 y funct7
    input  [1:0] Alu_op,
    input  [2:0] funct3,
    input        op,
    input        funct7,
    output reg [2:0] Alu_Control
);

always @(*) begin
    case (Alu_op)
        2'b00: Alu_Control = 0;  // lw/sw
        2'b01: Alu_Control = 1;  // beq

        2'b10: begin
            case (funct3)
                3'b000: begin                  
                    if (op && funct7)
                        Alu_Control = 1;           // subtract
                    else
                        Alu_Control = 0;         // add
                end

                3'b010: Alu_Control = 3'b101;  //slt
                3'b110: Alu_Control = 3'b011;  //or 
                3'b111: Alu_Control = 3'b010;  //and

                default: Alu_Control = 0;
            endcase
        end

        default: Alu_Control = 0;
    endcase
end

endmodule