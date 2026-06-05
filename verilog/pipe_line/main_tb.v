module main_tb();

reg clk;
reg rst;

main dut(
    .clk(clk),
    .rst(rst)
);

always #5 clk = ~clk;

initial begin
    clk = 0;
    rst = 1;

    #12;
    rst = 0;

    #220;

    
    
end

initial begin
    $dumpfile("pipeline_cpu.vcd");
    $dumpvars(0, main_tb);
end

initial begin
    $display("PCF | InstrD | ALUResultE | ResultW | StallF | StallD | FlushD | FlushE | ForwardAE | ForwardBE");
    $monitor("%h | %h | %h | %h | %b | %b | %b | %b | %b | %b",
        $time,
        dut.PCF,
        dut.InstrD,
        dut.ALUResultE,
        dut.ResultW,
        dut.StallF,
        dut.StallD,
        dut.FlushD,
        dut.FlushE,
        dut.ForwardAE,
        dut.ForwardBE);
end

endmodule
