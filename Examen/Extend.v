module Extend( // Extiende inmediatos de 12 o 20 a 32
    input [23:0] instr,
    output reg [31:0] ImmExt
);

wire [3:0] opcode;

assign opcode = instr[23:20];

always @(*)
begin
    if(opcode == 7)
        ImmExt = {{12{instr[19]}}, instr[19:0]};
    else
        ImmExt = {{20{instr[11]}}, instr[11:0]};
end

endmodule
