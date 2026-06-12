module PC( // Guarda la direccion de la instruccion actual
    input clk,
    input rst,
    input PCWrite,
    input [31:0] pc_next,
    output reg [31:0] pc
);

always @(posedge clk or posedge rst)
begin
    if(rst)
        pc <= 0;
    else if(PCWrite)
        pc <= pc_next;
end

endmodule
