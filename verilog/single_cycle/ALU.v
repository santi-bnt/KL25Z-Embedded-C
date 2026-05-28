module ALU (                      // Hace las operaciones 
    input [0:31] A,B,
    input [2:0] Alucontrol,
    output reg [31:0] result,
    output  Zero
);

always @(*) begin
    case (Alucontrol)
        3'b000: result = A+B ;   //  add
        3'b001: result = A-B ;   // subtract
        3'b010: result = A & B;  // and
        3'b011: result = A | B;  // or
        3'b101: result = ($signed(A) < $signed(B)) ? 1 : 0; //slt
        // signed es para usar el bit de signo
        // (si a es menor que b)  1 si no 0
        default: result = 0;
    endcase
end

assign Zero  = (result == 0);
    
endmodule
