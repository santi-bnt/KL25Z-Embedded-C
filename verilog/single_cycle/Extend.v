module Extend(               // toma la instruccion y la hace de 32 bits 
    input [31:7] instr,
    input [1:0] ImmSrc,
    output reg [31:0] ImmExt
);

always @(*) 
begin
    case(ImmSrc)

    //I-type
    2'b00:
        ImmExt = {{20{instr[31]}}, instr [31:20]};

    //S-type
    2'b01:
        ImmExt = {{20{instr[31]}}, instr [31:25], instr [11:7]};

    //B-type
    2'b10:
        ImmExt = {{19{instr[31]}}, instr [31], instr [7], instr[30:25], instr[11:8],1'b0};

    //J-type
    2'b11: 
        ImmExt = {{12{instr[31]}}, instr[31], instr[19:12], instr[20], instr[30:21], 1'b0};

    default: ImmExt = 0;

    endcase
end

endmodule