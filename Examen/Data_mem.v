module Data_mem( // Memoria de datos
    input clk,
    input WE,
    input [31:0] A,
    input [31:0] WD,
    output [31:0] RD
);

reg [31:0] memory [0:255];
integer i;

initial
begin
    for(i = 0; i < 256; i = i + 1)
        memory[i] = 0;
end

assign RD = memory[A[7:0]];

always @(posedge clk)
begin
    if(WE)
        memory[A[7:0]] <= WD;
end

endmodule
