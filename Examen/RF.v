module RF( // 16 registros R0 a R15. R0 siempre vale 0
    input clk,
    input WE3,
    input [3:0] A1,
    input [3:0] A2,
    input [3:0] A3,
    input [31:0] WD3,
    output reg [31:0] RD1,
    output reg [31:0] RD2
);

reg [31:0] REG [0:15];
integer i;

initial
begin
    for(i = 0; i < 16; i = i + 1)
        REG[i] = 0;
end

always @(*)
begin
    if(A1 == 0)
        RD1 = 0;
    else
        RD1 = REG[A1];

    if(A2 == 0)
        RD2 = 0;
    else
        RD2 = REG[A2];
end

always @(posedge clk)
begin
    REG[0] <= 0;

    if(WE3 && A3 != 0)
        REG[A3] <= WD3;
end

endmodule
