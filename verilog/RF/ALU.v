module ALU (
    input [0:31] A,B,
    input [0:2] Alucontrol,
    output reg [0:31] result,
    output reg Zero
);

always @(*) begin
    case (Alucontrol)
        3'b000: result = A+B ;   //  add
        3'b001: result = A-B ;   // subtract
        3'b010: result = A & B;  // and
        3'b011: result = A | B;  // or
        3'b101: result = A << B;    //slt
        default: result = 0;
    endcase
    if (result == 0)
        Zero = 1;
    else
        Zero = 0;
end
    
endmodule