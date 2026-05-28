module mux3(                    //mux que use tres datos
    input [31:0] a,
    input [31:0] b,
    input [31:0] c,
    input [1:0] select,
    output reg [31:0] out
);

always @(*)begin
    case(select)
        2'b00: out = a;       //AluResult
        2'b01: out = b;       //ReadData
        2'b10: out = c;       // Pc + 4 para usar jal
        default:
        out = 0;
    endcase
end
endmodule