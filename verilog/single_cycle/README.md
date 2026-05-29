# RISC-V Single-Cycle Processor

This project implements a basic RISC-V single-cycle processor in Verilog.

## Objective

The objective of this project is to implement a simple RISC-V processor using a single-cycle architecture. In this type of processor, each instruction is executed in one clock cycle.

The design includes the main functional blocks of a basic RV32I datapath: Program Counter, Instruction Memory, Register File, Immediate Generator, Control Unit, ALU Decoder, ALU, Data Memory, multiplexers, adders, and branch comparison logic.

## Project Structure

```text
single_cycle/
│
├── main.v
├── main_tb.v
├── PC.v
├── Instruction_memory.v
├── RF.v
├── Extend.v
├── Control_unit.v
├── main_decoder.v
├── ALU_decoder.v
├── ALU.v
├── Data_mem.v
├── mux.v
├── mux3.v
├── adder.v
├── branch_comparator.v
├── instrMem.hex
└── README.md
```

## Modules

| File | Description |
|---|---|
| `main.v` | Main module that connects all processor blocks |
| `main_tb.v` | Testbench used to simulate the processor |
| `PC.v` | Program Counter. Stores the current instruction address |
| `Instruction_memory.v` | Stores the program instructions |
| `RF.v` | Register File with 32 registers of 32 bits |
| `Extend.v` | Generates and extends immediate values to 32 bits |
| `Control_unit.v` | Connects the main decoder and the ALU decoder |
| `main_decoder.v` | Generates the main control signals based on the opcode |
| `ALU_decoder.v` | Generates the ALU control signal |
| `ALU.v` | Executes arithmetic and logic operations |
| `Data_mem.v` | Data memory used by `lw` and `sw` instructions |
| `mux.v` | 2-input multiplexer |
| `mux3.v` | 3-input multiplexer used to support `jal` |
| `adder.v` | Adds two 32-bit values |
| `branch_comparator.v` | Compares two register values for branch instructions |

## Basic Datapath

```text
PC
↓
Instruction Memory
↓
Control Unit
↓
Register File
↓
ALU
↓
Data Memory
↓
Write Back
```

## General Operation

1. The PC stores the address of the current instruction.
2. The Instruction Memory receives the PC address and outputs the instruction.
3. The Control Unit reads the opcode and generates the control signals.
4. The Register File reads the source registers.
5. The Extend module generates the immediate value when needed.
6. The ALU performs the selected operation.
7. The Data Memory is used for load and store instructions.
8. The final result is written back into the Register File when `RegWrite` is active.
9. The next PC is selected between `PC + 4` and the branch or jump target.

## Supported Instructions

```text
lw
sw
add
sub
and
or
slt
addi
beq
jal
```

## Control Signals

| Signal | Description |
|---|---|
| `RegWrite` | Enables writing into the Register File |
| `MemWrite` | Enables writing into Data Memory |
| `AluSrc` | Selects the second ALU input: register value or immediate |
| `ResultSrc` | Selects the value written back to the Register File |
| `ImmSrc` | Selects the immediate format |
| `Alu_op` | Indicates the general ALU operation type |
| `Alu_control` | Indicates the exact operation performed by the ALU |
| `PCSrc` | Selects the next PC value |
| `Zero` | Indicates if the ALU result is zero |

## ALU Operations

| `Alu_control` | Operation |
|---|---|
| `000` | ADD |
| `001` | SUB |
| `010` | AND |
| `011` | OR |
| `101` | SLT |

## Result Source

```text
00 -> ALUResult
01 -> ReadData
10 -> PC + 4
```

## Test Program

The instruction memory loads the program from:

```text
test_program.mem
```

Example program:

```text
00500093
00A00113
002081B3
```

Equivalent RISC-V assembly:

```assembly
addi x1, x0, 5
addi x2, x0, 10
add  x3, x1, x2
```

Expected result:

```text
x1 = 5
x2 = 10
x3 = 15
```

## Testbench

The testbench file is:

```text
main_tb.v
```

The generated waveform file is:

```text
single_cycle_cpu.vcd
```

The testbench generates the clock and reset signals, prints important signals in the terminal, and creates a VCD file to observe the simulation in GTKWave.

## Running the Simulation

Compile the project with Icarus Verilog:

```cmd
iverilog -o single_cycle_cpu main_tb.v main.v PC.v Instruction_memory.v RF.v Extend.v Control_unit.v main_decoder.v ALU_decoder.v ALU.v Data_mem.v mux.v mux3.v adder.v branch_comparator.v
```

Run the simulation:

```cmd
vvp single_cycle_cpu
```

Open the waveform file in GTKWave:

```cmd
gtkwave single_cycle_cpu.vcd
```

## Signals to Observe

```text
clk
rst
pc
pc_next
pc_plus4
pc_target
instr
RD1
RD2
ImmExt
SrcB
ALUResult
ReadData
Result
Zero
PCSrc
MemWrite
RegWrite
AluSrc
ResultSrc
ImmSrc
Alu_control
```

## Expected Simulation Behavior

1. The PC starts at 0 after reset.
2. The Instruction Memory outputs the instruction at the current PC.
3. The Control Unit generates the correct control signals.
4. The Register File reads the required registers.
5. The ALU performs the selected operation.
6. The result is written back to the Register File if `RegWrite` is active.
7. The PC updates to the next instruction.
8. The VCD file is generated for waveform analysis.

## Notes

This is a basic educational implementation of a RISC-V single-cycle processor. The design focuses on understanding the datapath, control signals, and instruction execution flow.
