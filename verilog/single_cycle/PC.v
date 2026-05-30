module PC(                        //Program_counter: guarda la direccion actual y avanza de 4 en 4 bytes
    input clk, rst,
    input [31:0] pc_next,
    output reg [31:0] pc
);

always @(posedge clk or posedge rst)
begin
    if (rst == 1)
        pc <= 32'd0;
    else
        pc <= pc_next;
end

endmodule