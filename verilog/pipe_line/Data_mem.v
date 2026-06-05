module Data_mem(          //Donde se guardan o se leen datos con lw o sw
    input clk,WE,
    input [31:0] A,WD,
    output [31:0] RD
);

reg [31:0] memory [0:255];
integer i;

initial begin
    for (i = 0; i < 256; i = i + 1)
        memory[i] = 0;
end

assign RD = memory[A[31:2]];

always @(posedge clk)
    begin
        if (WE)        
            memory[A[31:2]] <= WD;
    end

endmodule
