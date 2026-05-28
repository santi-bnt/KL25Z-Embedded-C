module mux (               // agarra dos entradas segun la señal de control
 input [31:0] a,b,
 input select,
 output [31:0] out
);

//if select 0 out = b if select 1 out = a
assign out = select ? b :a;  

endmodule