module RF(                         // Register_file: Guarda los 32 registros se pueden leer dos registros y escribir uno
    input clk,WE3,
    input[4:0] A1,A2,A3,
    input [31:0] WD3,
    output  [31:0] RD1,RD2
);

reg [31:0] REG [31:0];


assign RD1 = (A1 == 0) ? 0 : REG[A1];
assign RD2 = (A2 == 0) ? 0 : REG[A2];            
// si los a1 y a2 apuntan a x0 entonces es 0 si no lee el registro

always @(posedge clk)
begin
    if (WE3 ==1 && A3 != 0)
        REG[A3] = WD3;    // escribe lo de WD3 si a3 no apunta a x0
end

endmodule







