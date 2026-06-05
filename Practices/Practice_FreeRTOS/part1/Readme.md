# Parte 1 - FreeRTOS con Tareas y Variables Globales

## Reflexión sobre tareas y variables globales

El uso de tareas permite dividir el programa en funciones más ordenadas, por ejemplo una tarea para leer luz, otra para temperatura, otra para el botón y otra para mostrar datos. Esto hace que el código sea más fácil de entender. Sin embargo, usar variables globales puede ser una desventaja porque varias tareas pueden leer o modificar los mismos datos al mismo tiempo, lo que puede causar errores difíciles de detectar en sistemas más grandes.

## Descripción

En esta parte se implementó un sistema con FreeRTOS usando varias tareas para leer dos potenciómetros y un botón. El Potenciómetro 1 simula el sensor de luz, el Potenciómetro 2 simula el sensor de temperatura y el botón funciona como entrada digital.

## Conexiones

### Potenciómetro 1 - Luz

```text
Extremo 1  -> 3.3V
Extremo 2  -> GND
Centro     -> PTB1 / ADC0_SE9
```

### Potenciómetro 2 - Temperatura

```text
Extremo 1  -> 3.3V
Extremo 2  -> GND
Centro     -> PTB2 / ADC0_SE12
```

### Botón

```text
PTB0 ---- botón ---- GND
```

Lógica del botón:

```text
1 = no presionado
0 = presionado
```

## Liga al video

```text
https://drive.google.com/file/d/1OjviULnFDysXgwU4z_qHrqXBNpSONq-W/view?usp=sharing
```