module HazardUnit( // Detecta riesgos del pipeline y decide si se usa stall, flush o forwarding
    input [4:0] Rs1D,
    input [4:0] Rs2D,
    input [4:0] Rs1E,
    input [4:0] Rs2E,
    input [4:0] RdE,
    input [4:0] RdM,
    input [4:0] RdW,
    input [1:0] ResultSrcE,
    input RegWriteM,
    input RegWriteW,
    input PCSrcE,
    output StallF,
    output StallD,
    output FlushD,
    output FlushE,
    output reg [1:0] ForwardAE,
    output reg [1:0] ForwardBE
);

wire lwStall;

assign lwStall = ResultSrcE[0] && ((Rs1D == RdE) || (Rs2D == RdE)) && (RdE != 0);
assign StallF = lwStall;
assign StallD = lwStall;
assign FlushD = PCSrcE;
assign FlushE = lwStall || PCSrcE;

always @(*) begin
    if ((Rs1E != 0) && (Rs1E == RdM) && RegWriteM)
        ForwardAE = 2;
    else if ((Rs1E != 0) && (Rs1E == RdW) && RegWriteW)
        ForwardAE = 1;
    else
        ForwardAE = 0;

    if ((Rs2E != 0) && (Rs2E == RdM) && RegWriteM)
        ForwardBE = 2;
    else if ((Rs2E != 0) && (Rs2E == RdW) && RegWriteW)
        ForwardBE = 1;
    else
        ForwardBE = 0;
end

endmodule
