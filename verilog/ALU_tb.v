module ALU_tb(
);
reg [0:31] A,B;
reg [0:2] Alucontrol;
wire [0:31] result;
wire Zero;

ALU DUT(.A(A),.B(B),.Alucontrol(Alucontrol),.result(result),.Zero(Zero));

initial begin
    repeat(80)
    begin 
        A = $random %100;
        B = $random %100;
        Alucontrol = $random %6; // 4 NO DEFINIFO RESULT = 0
        #10;
end 
end

wire [63:0] timestamp;
assign timestamp = $time;

initial
begin
    $monitor("%4t | %d | %d | %d | %d | %d | \n ", timestamp, A, B, Alucontrol, result, Zero);
end

initial begin
    $dumpfile("ALU_TB.vcd");
    $dumpvars(0, ALU_tb);
end
endmodule