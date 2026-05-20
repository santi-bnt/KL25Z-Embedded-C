module main_decoder(
    input [6:0] op,
    input Zero
    output reg [1:0] ResultSrc,
    output reg PCSrc,
    output reg MemWrite,
    output reg AluSrc,
    output reg [1:0] ImmSrc,
    output reg RegWrite,
    output reg [1:0] Alu_op,
    output branch,
    output jump

);

always @(*)begin
    case(op )
        3: begin
            RegWrite = b'1;
            ImmSrc = b'00;
            AluSrc = b'1;
            MemWrite = b'0;
            ResultSrc = b'01;
            branch = b'0;
            Alu_op = b'00;
            jump = b'0;
        end
        35: begin
            RegWrite =01;
            ImmSrc = 01;
            AluSrc = 1;
            MemWrite = 1;
            ResultSrc = b'xx;
            branch = 0
            Alu_op = 00
            jump = 0
        end
        51: begin
            RegWrite = 1;
            ImmSrc = b'xx;
            AluSrc = 0;
            MemWrite = 0;
            ResultSrc = 0;
            branch = 0;
            Alu_op = b'10;
            jump = 0;
        end
        99: begin
            RegWrite = 0;
            ImmSrc = b'10;
            AluSrc = 0;
            MemWrite = 0;
            ResultSrc = b'xx;
            branch = 1;
            Alu_op = 01;
            jump = 0;
        end
        19: begin
            RegWrite = 1;
            ImmSrc = 00;
            AluSrc = 1;
            MemWrite = 0;
            ResultSrc = 00;
            branch = 0;
            Alu_op = b'10;
            jump = 0;
        end
        111: begin
            RegWrite = 1;
            ImmSrc = b'11;
            AluSrc = b'x;
            MemWrite = 0;
            ResultSrc = b'10;
            branch = 0;
            Alu_op = b'xx;
            jump = 1;
        end
        default:begin
            RegWrite = 0;
            ImmSrc = b'00;
            AluSrc = b'0;
            MemWrite = 0;
            ResultSrc = b'00;
            branch = 0;
            Alu_op = b'00;
            jump = 0;
        end


endcase
end

endmodule