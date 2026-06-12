module ALU( // Hace add, sub y and. Tambien sirve para comparar beq con resta
    input [31:0] A,
    input [31:0] B,
    input [1:0] AluControl,
    output reg [31:0] Result,
    output Zero
);

always @(*)
begin
    case(AluControl)
        0: Result = A + B;
        1: Result = A - B;
        2: Result = A & B;
        default: Result = 0;
    endcase
end

assign Zero = (Result == 0);

endmodule
