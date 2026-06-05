module main_decoder(      //lee el opcode  y le da las senales princiaples al procesador
    input [6:0] op,
    input Zero,
    output reg [1:0] ResultSrc,
    output reg PCSrc,
    output reg MemWrite,
    output reg AluSrc,
    output reg [1:0] ImmSrc,
    output reg RegWrite,
    output reg [1:0] Alu_op
);

reg branch;
reg jump;

always @(*)begin
    case(op )
        3: begin                   //lw
            RegWrite = 1;
            ImmSrc = 0;
            AluSrc = 1;
            MemWrite = 0;
            ResultSrc = 1;
            branch = 0;
            Alu_op = 0;
            jump = 0;
        end
        35: begin                     //sw
            RegWrite =0;
            ImmSrc = 1;
            AluSrc = 1;
            MemWrite = 1;
            ResultSrc = 0;
            branch = 0;
            Alu_op = 0;
            jump = 0;
        end 
        51: begin                   //r-type
            RegWrite = 1;
            ImmSrc = 0;
            AluSrc = 0;
            MemWrite = 0;
            ResultSrc = 0;
            branch = 0;
            Alu_op = 2;
            jump = 0;
        end
        99: begin                  //beq
            RegWrite = 0;
            ImmSrc = 2;
            AluSrc = 0;
            MemWrite = 0;
            ResultSrc = 0;
            branch = 1;
            Alu_op = 1;
            jump = 0;
        end
        19: begin              //I-type
            RegWrite = 1;
            ImmSrc = 0;
            AluSrc = 1;
            MemWrite = 0;
            ResultSrc = 0;
            branch = 0;
            Alu_op = 2;
            jump = 0;
        end
        111: begin         //jal
            RegWrite = 1;
            ImmSrc = 3;
            AluSrc = 0;
            MemWrite = 0;
            ResultSrc = 2;
            branch = 0;
            Alu_op = 0;
            jump = 1;
        end
        default:begin
            RegWrite = 0;
            ImmSrc = 0;
            AluSrc = 0;
            MemWrite = 0;
            ResultSrc = 0;
            branch = 0;
            Alu_op = 0;
            jump = 0;
        end

endcase
    PCSrc = (branch&&Zero) || jump;  // si branch y zero se usa el beq          
                                     // si jump es 1 se usa el jal        
                                     // si no se sigue de 4 en 4
end

endmodule
