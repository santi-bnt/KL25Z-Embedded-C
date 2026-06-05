module branch_comparator( // Compara dos registros para saber si son iguales
    input [31:0] A,
    input [31:0] B,
    output Equal
);

assign Equal = (A == B);

endmodule