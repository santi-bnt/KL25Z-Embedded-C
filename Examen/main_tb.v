module main_tb();

reg clk;
reg rst;

main dut(
    .clk(clk),
    .rst(rst)
);

always #5 clk = ~clk;

initial
begin
    clk = 0;
    rst = 1;
    
    #12;
    rst = 0;

    #500;

    $finish;
end

initial begin
    $dumpfile("rv_mini_multicycle.vcd");
    $dumpvars(0, main_tb);
end

initial begin
    $display("pc | state | IR | ALUOut | MDR | R3 | R6 | R10 | MEM20");
    $monitor("%h | %d | %h | %d | %d | %d | %d | %d | %d",
        dut.pc,
        dut.state,
        dut.IR,
        dut.ALUOut,
        dut.MDR,
        dut.register_file.REG[3],
        dut.register_file.REG[6],
        dut.register_file.REG[10],
        dut.dmem.memory[20]);
end

endmodule
