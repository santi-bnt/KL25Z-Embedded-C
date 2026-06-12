# KL25Z Embedded C

Este repositorio junta practicas y proyectos hechos para la tarjeta **FRDM-KL25Z** y tambien ejercicios de arquitectura de computadores en **Verilog**.

La parte de embedded usa programacion en C para trabajar con GPIO, ADC, PWM, LCD, teclado matricial, interrupciones y FreeRTOS.

La parte de Verilog contiene procesadores educativos tipo RISC-V: single-cycle, pipeline y una arquitectura mini multiciclo para el examen.

## Estructura del repositorio

```text
KL25Z-Embedded-C/
|
|-- Practices/
|   |-- Practice1/
|   |-- Practice1_FreeRTOS/
|   |-- Practice2_FreeRTOS/
|   |-- Practice3/
|   |-- Practice4/
|   |-- Practice_potenciometer/
|   |-- Practice_temperature/
|   `-- prac_pwm/
|
|-- verilog/
|   |-- ALU/
|   |-- single_cycle/
|   `-- pipe_line/
|
|-- Examen/
|-- Mini_challenges/
|-- Summatory_RISCV/
`-- Proyect_MATT/
```

## Practices

La carpeta `Practices` contiene ejercicios para la **FRDM-KL25Z**.

| Carpeta | Descripcion |
|---|---|
| `Practice1` | Control de LED RGB usando teclado matricial y LCD. |
| `Practice1_FreeRTOS` | Primeras versiones usando FreeRTOS con tareas. |
| `Practice2_FreeRTOS` | FreeRTOS con queues, mutexes, semaforos e interrupciones. |
| `Practice3` | Practica con teclado, LCD, timers e interrupciones. |
| `Practice4` | Control de estados running/paused con interrupciones. |
| `Practice_potenciometer` | Lectura de potenciometro usando ADC. |
| `Practice_temperature` | Lectura de sensor de temperatura y salida en LCD. |
| `prac_pwm` | Integracion de LCD, teclado, ADC, interrupciones y PWM. |

## Verilog

La carpeta `verilog` contiene modulos y procesadores hechos en Verilog.

| Carpeta | Descripcion |
|---|---|
| `ALU` | ALU basica y testbench. |
| `single_cycle` | Procesador RISC-V educativo single-cycle. |
| `pipe_line` | Procesador RISC-V educativo con pipeline de 5 etapas. |

## Examen

La carpeta `Examen` contiene la arquitectura **RV-Mini Multicycle**.

Esta arquitectura usa:

```text
instrucciones de 24 bits
16 registros R0 a R15
FSM de control
ALU reutilizada en varios ciclos
load, store, jump, add, sub, and, addi, beq
```

Tambien incluye imagenes del diseno en:

```text
Examen/images/
```

## Comandos utiles

### Correr Examen con Icarus Verilog

Desde la carpeta `Examen`:

```cmd
iverilog -o mini_multicycle.out main_tb.v main.v PC.v Instruction_memory.v RF.v Extend.v Control_unit.v ALU.v Data_mem.v
vvp mini_multicycle.out
```

### Correr single-cycle

Desde `verilog/single_cycle`:

```cmd
iverilog -o single_cycle_cpu main_tb.v main.v PC.v Instruction_memory.v RF.v Extend.v Control_unit.v main_decoder.v ALU_decoder.v ALU.v Data_mem.v mux.v mux3.v adder.v branch_comparator.v
vvp single_cycle_cpu
```

### Correr pipeline

Desde `verilog/pipe_line`:

```cmd
iverilog -o pipeline_cpu main_tb.v main.v IF_stage.v ID_stage.v EX_stage.v MEM_stage.v WB_stage.v HazardUnit.v Instruction_memory.v PC.v RF.v Extend.v Control_unit.v main_decoder.v ALU_decoder.v ALU.v Data_mem.v mux.v mux3.v adder.v branch_comparator.v
vvp pipeline_cpu
```

## Archivos generados

Estos archivos normalmente son resultado de simulaciones y no son necesarios para entender el codigo:

```text
*.out
*.vcd
work/
transcript
vsim.wlf
```

Se pueden borrar y volver a generar corriendo los testbench.

## Herramientas usadas

```text
C / Embedded C
FreeRTOS
Verilog
Icarus Verilog
ModelSim / Questa
FRDM-KL25Z
```

## Nota

El repositorio esta pensado como material de practica. Muchos archivos tienen comentarios simples para explicar que hace cada bloque y poder seguir el funcionamiento paso por paso.
