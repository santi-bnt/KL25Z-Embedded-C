module Control_unit( // FSM del procesador multiciclo
    input clk,
    input rst,
    input [3:0] opcode,
    input Zero,

    output reg PCWrite,
    output reg IRWrite,
    output reg RegWrite,
    output reg MemRead,
    output reg MemWrite,
    output reg MDRWrite,
    output reg ALUOutWrite,
    output reg ALUSrcA,
    output reg [1:0] ALUSrcB,
    output reg [1:0] ALUOp,
    output reg MemToReg,
    output reg Branch,
    output reg Jump,
    output reg [1:0] PCSrc,
    output reg [2:0] state
);

// opcodes en numeros para que sea facil verlos
localparam ADD   = 0;
localparam SUB   = 1;
localparam ANDD  = 2;
localparam ADDI  = 3;
localparam LOAD  = 4;
localparam STORE = 5;
localparam BEQ   = 6;
localparam JUMP  = 7;

// estados de la FSM
localparam FETCH  = 0;
localparam DECODE = 1;
localparam EXEC   = 2;
localparam MEM    = 3;
localparam WB     = 4;
localparam JUMP_S = 5;

reg [2:0] next_state;

always @(posedge clk or posedge rst)
begin
    if(rst)
        state <= FETCH;
    else
        state <= next_state;
end

always @(*)
begin
    case(state)
        FETCH:
            next_state = DECODE;

        DECODE:
        begin
            if(opcode == JUMP)
                next_state = JUMP_S;
            else
                next_state = EXEC;
        end

        EXEC:
        begin
            if(opcode == LOAD || opcode == STORE)
                next_state = MEM;
            else if(opcode == BEQ)
                next_state = FETCH;
            else
                next_state = WB;
        end

        MEM:
        begin
            if(opcode == LOAD)
                next_state = WB;
            else
                next_state = FETCH;
        end

        WB:
            next_state = FETCH;

        JUMP_S:
            next_state = FETCH;

        default:
            next_state = FETCH;
    endcase
end

always @(*)
begin
    PCWrite = 0;
    IRWrite = 0;
    RegWrite = 0;
    MemRead = 0;
    MemWrite = 0;
    MDRWrite = 0;
    ALUOutWrite = 0;
    ALUSrcA = 0;
    ALUSrcB = 0;
    ALUOp = 0;
    MemToReg = 0;
    Branch = 0;
    Jump = 0;
    PCSrc = 0;

    case(state)
        FETCH:
        begin
            IRWrite = 1;
            PCWrite = 1;
            ALUSrcA = 0;      // PC
            ALUSrcB = 2;      // +1
            ALUOp = 0;        // suma
            PCSrc = 0;        // resultado de ALU
        end

        EXEC:
        begin
            ALUSrcA = 1;      // registro A
            ALUOutWrite = 1;

            if(opcode == ADD || opcode == SUB || opcode == ANDD || opcode == BEQ)
                ALUSrcB = 0;  // registro B
            else
                ALUSrcB = 1;  // inmediato

            if(opcode == SUB || opcode == BEQ)
                ALUOp = 1;
            else if(opcode == ANDD)
                ALUOp = 2;
            else
                ALUOp = 0;

            if(opcode == BEQ)
            begin
                Branch = 1;
                PCSrc = 1;    // PC + offset
            end
        end

        MEM:
        begin
            if(opcode == LOAD)
            begin
                MemRead = 1;
                MDRWrite = 1;
            end
            else if(opcode == STORE)
                MemWrite = 1;
        end

        WB:
        begin
            if(opcode == LOAD)
            begin
                RegWrite = 1;
                MemToReg = 1;
            end
            else if(opcode != STORE && opcode != BEQ && opcode != JUMP)
            begin
                RegWrite = 1;
                MemToReg = 0;
            end
        end

        JUMP_S:
        begin
            Jump = 1;
            PCWrite = 1;
            PCSrc = 2;        // PC + offset largo
        end
    endcase
end

endmodule
