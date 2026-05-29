module RF(                          // Register_file: Guarda los 32 registros se pueden leer dos registros y escribir uno
    input clk, WE3,
    input [4:0] A1, A2, A3,
    input [31:0] WD3,
    output reg [31:0] RD1, RD2
);

reg [31:0] REG [31:0]; 


always @(*)
begin
    if (A1 == 0)
        RD1 = 0;
    else
        RD1 = REG[A1];

    if (A2 == 0)
        RD2 = 0;
    else
        RD2 = REG[A2];
end

always @(posedge clk)
begin
    if (WE3 == 1 && A3 != 0)
        REG[A3] <= WD3;
end

endmodule