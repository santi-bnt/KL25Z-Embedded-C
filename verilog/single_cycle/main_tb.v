module main_tb();                      
reg clk, rst;

main dut(.clk(clk), .rst(rst));

always #5 clk = ~clk;

initial begin
    clk = 0;
    rst = 1;

    #10;
    rst = 0;

    #100;

    $finish;
end


initial begin
    $dumpfile("single_cycle_cpu.vcd");
    $dumpvars(0, main_tb);
end


initial begin
    $display("clk | rst | PC | Instr | ALUResult | Result | RegWrite | MemWrite | PCSrc");
    $monitor("%b | %b | %h | %h | %h | %h | %b | %b | %b",
    clk,rst,dut.pc,dut.instr,dut.ALUResult,dut.Result,dut.RegWrite,dut.MemWrite,dut.PCSrc);
end

endmodule