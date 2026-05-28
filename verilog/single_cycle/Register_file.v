module Register_file(
    input clk,WE3
    input[4:0] A1,A2,A3
    input [31:0] WD3,
    output [31:0] RD1,RD2
);

reg [31:0] REG [31:0];

always @(posedge clk)
begin
    RD1= REG[A1];
    RD2= REG[A2];
end
begin
    if (WE3 ==1)
        REG[A3] = WD3;
end

endmodule







endmodule