# Procesador RISC-V Pipeline de 5 Etapas

## Datos del proyecto

**Materia:** Arquitectura / Diseno Digital  
**Proyecto:** Procesador RISC-V con pipeline de 5 etapas  
**Equipo:** Escribir aqui los nombres de los integrantes  
**Repositorio:** Carpeta `verilog/pipe_line`

## Descripcion general

Este proyecto implementa un procesador RISC-V educativo en Verilog usando una arquitectura pipeline de 5 etapas. El diseno toma como base el procesador `single_cycle` del repositorio y conserva nombres de modulos ya usados, como `ALU`, `RF`, `Extend`, `Control_unit`, `Data_mem`, `Instruction_memory`, `mux`, `mux3` y `adder`.

El procesador soporta un subconjunto de instrucciones RV32I:

```text
lw, sw, add, sub, and, or, slt, addi, beq, jal
```

La meta del pipeline es permitir que varias instrucciones avancen al mismo tiempo por diferentes etapas. Mientras una instruccion se ejecuta en la ALU, otra se decodifica, otra se busca en memoria de instrucciones y otras pueden estar en memoria o write-back.

## Estructura del codigo

```text
pipe_line/
|-- main.v
|-- IF_stage.v
|-- ID_stage.v
|-- EX_stage.v
|-- MEM_stage.v
|-- WB_stage.v
|-- HazardUnit.v
|-- main_tb.v
|-- instrMem.hex
|-- modelsim.do
|-- Instruction_memory.v
|-- RF.v
|-- Extend.v
|-- Control_unit.v
|-- main_decoder.v
|-- ALU_decoder.v
|-- ALU.v
|-- Data_mem.v
|-- mux.v
|-- mux3.v
|-- adder.v
|-- branch_comparator.v
|-- PC.v
`-- README.md
```

## Modulos principales

| Modulo | Funcion |
|---|---|
| `main.v` | Conecta todas las etapas y contiene los registros pipeline IF/ID, ID/EX, EX/MEM y MEM/WB. |
| `IF_stage.v` | Busca instrucciones, actualiza el PC y calcula `PC + 4`. |
| `ID_stage.v` | Decodifica la instruccion, lee el Register File y genera inmediatos. |
| `EX_stage.v` | Ejecuta la ALU, calcula branch/jump target y aplica forwarding. |
| `MEM_stage.v` | Lee o escribe memoria de datos. |
| `WB_stage.v` | Selecciona el dato final que se escribe al Register File. |
| `HazardUnit.v` | Detecta riesgos de datos/control, genera stalls, flushes y forwarding. |
| `main_tb.v` | Testbench integral del procesador completo. |

No se agregaron testbench separados para cada modulo elemental. En su lugar, el testbench integral documentado en `main_tb.v` prueba el comportamiento completo del procesador, incluyendo forwarding, stall por `lw` y flush por branch.

## Pipeline de 5 etapas

### 1. IF - Instruction Fetch

La etapa IF mantiene el `PCF`, obtiene la instruccion `InstrF` desde `Instruction_memory` y calcula `PCPlus4F`. Si la unidad de hazard activa `StallF`, el PC se mantiene. Si `PCSrcE` se activa por branch o jump, el PC toma `PCTargetE`.

### 2. ID - Instruction Decode

La etapa ID recibe `InstrD`, separa campos como `rs1`, `rs2` y `rd`, genera senales de control con `Control_unit`, lee el Register File `RF` y extiende inmediatos con `Extend`.

El Register File escribe en flanco de bajada para que el resultado de WB pueda estar disponible para una instruccion que se decodifica en el mismo ciclo.

### 3. EX - Execute

La etapa EX ejecuta operaciones aritmeticas/logicas con `ALU`, calcula `PCTargetE = PCE + ImmExtE` y decide si un `beq` o `jal` cambia el PC. Tambien aplica forwarding para evitar stalls innecesarios.

### 4. MEM - Memory

La etapa MEM usa `Data_mem` para instrucciones `lw` y `sw`. En `lw`, el dato leido pasa hacia WB. En `sw`, se escribe `WriteDataM` en la direccion calculada por la ALU.

### 5. WB - Write Back

La etapa WB selecciona entre:

```text
00 -> ALUResultW
01 -> ReadDataW
10 -> PCPlus4W
```

El resultado seleccionado `ResultW` se escribe en `RF` cuando `RegWriteW` esta activo.

## Unidad de hazard

La `HazardUnit` controla tres casos principales:

| Caso | Solucion |
|---|---|
| Dependencia ALU-ALU | Forwarding desde MEM o WB hacia EX. |
| Dependencia `lw` seguida de uso inmediato | `StallF`, `StallD` y `FlushE` para insertar una burbuja. |
| Branch/jump tomado | `FlushD` y `FlushE` para eliminar instrucciones incorrectas. |

Senales importantes:

```text
ForwardAE, ForwardBE
StallF, StallD
FlushD, FlushE
PCSrcE
```

## Programa de prueba

El archivo `instrMem.hex` contiene este programa:

```assembly
addi x1, x0, 5
addi x2, x0, 10
add  x3, x1, x2
sw   x3, 0(x0)
lw   x4, 0(x0)
add  x5, x4, x3
beq  x5, x5, skip
addi x6, x0, 99
skip:
or   x7, x5, x2
and  x8, x7, x5
```

Resultados esperados:

```text
x1 = 5
x2 = 10
x3 = 15
x4 = 15
x5 = 30
x6 = 0    // instruccion eliminada por flush
x7 = 30
x8 = 30
mem[0] = 15
```

## Simulacion en Questa / ModelSim

Desde la carpeta `verilog/pipe_line`:

```cmd
vlog main_tb.v main.v IF_stage.v ID_stage.v EX_stage.v MEM_stage.v WB_stage.v HazardUnit.v Instruction_memory.v PC.v RF.v Extend.v Control_unit.v main_decoder.v ALU_decoder.v ALU.v Data_mem.v mux.v mux3.v adder.v branch_comparator.v
vsim -c main_tb -do "run -all; quit"
```

Tambien se puede usar el archivo:

```cmd
vsim -do modelsim.do
```

## Simulacion con Icarus Verilog

Si Icarus Verilog esta instalado:

```cmd
iverilog -o pipeline_cpu main_tb.v main.v IF_stage.v ID_stage.v EX_stage.v MEM_stage.v WB_stage.v HazardUnit.v Instruction_memory.v PC.v RF.v Extend.v Control_unit.v main_decoder.v ALU_decoder.v ALU.v Data_mem.v mux.v mux3.v adder.v branch_comparator.v
vvp pipeline_cpu
```

## Resultados observados

La simulacion en Questa/ModelSim compilo sin errores ni warnings.

Salida final del testbench:

```text
x1 = 5
x2 = 10
x3 = 15
x4 = 15
x5 = 30
x6 = 0
x7 = 30
x8 = 30
mem[0] = 15
TEST PASSED: pipeline forwarding, load-use stall and branch flush worked.
```

Durante la simulacion se observaron estos eventos:

| Evento | Evidencia |
|---|---|
| Forwarding ALU-ALU | `ForwardAE` y `ForwardBE` se activan durante instrucciones dependientes. |
| Stall por `lw` | `StallF=1`, `StallD=1`, `FlushE=1` cuando `add x5, x4, x3` depende del `lw`. |
| Flush por `beq` | `FlushD=1`, `FlushE=1`, y `x6` queda en 0 porque `addi x6, x0, 99` se elimina. |

## Capturas de simulacion

Para el PDF del reporte se recomienda incluir:

1. Captura de terminal con `TEST PASSED`.
2. Captura de waveform mostrando `PCF`, `InstrD`, `ALUResultE`, `ResultW`.
3. Captura de hazard signals: `StallF`, `StallD`, `FlushD`, `FlushE`, `ForwardAE`, `ForwardBE`.
4. Captura de registros finales `x1` a `x8`.

El testbench genera el archivo:

```text
pipeline_cpu.vcd
```

Ese archivo puede abrirse en GTKWave o revisarse desde el visor de ondas de Questa/ModelSim.

## Conclusiones

El procesador pipeline implementa correctamente las 5 etapas clasicas IF, ID, EX, MEM y WB. Comparado con el procesador single-cycle, este diseno permite que varias instrucciones esten activas al mismo tiempo, por lo que se acerca mas al funcionamiento real de un procesador segmentado.

La unidad de hazard es esencial para que el pipeline funcione correctamente. El forwarding resuelve dependencias comunes sin detener el procesador, mientras que el stall por `lw` evita usar un dato que todavia no esta disponible. Los flushes limpian instrucciones incorrectas cuando un branch o jump cambia el PC.

El testbench integral valida el comportamiento principal del procesador y muestra resultados correctos en registros, memoria y senales de control del pipeline.
