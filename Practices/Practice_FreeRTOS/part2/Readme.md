# Parte 2 - FreeRTOS con Queues

## Ventajas de usar queues

Las queues permiten comunicar tareas de forma más ordenada y segura mediante mensajes. Esto evita depender directamente de variables globales y reduce errores cuando varias tareas trabajan al mismo tiempo. Además, hacen que el código sea más modular, fácil de mantener y más sencillo de depurar.

## Descripción

En esta parte se implementó el mismo sistema de la Parte 1, pero usando queues para comunicar las tareas. Los potenciómetros y el botón envían sus valores como mensajes a una queue, y otra tarea recibe esos datos para procesarlos.

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
https://drive.google.com/file/d/1hdm1lHsiEoUnTVAVh9BfP4-YANX8-wPM/view?usp=sharing
```