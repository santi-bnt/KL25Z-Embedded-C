# Parte 1 - Basic Concurrency

## Descripcion

En esta parte se implementa una version basica con tareas de FreeRTOS, variables globales y polling.

Esta arquitectura funciona, pero intencionalmente deja visibles dos problemas:

- Las tareas comparten datos usando variables globales.
- El boton se revisa periodicamente con polling.
- El ADC0 no esta protegido si mas de una tarea lo usa.

Estos problemas se corrigen en las partes siguientes.

## Conexiones

### Potenciometro 1 - Luz

```text
Extremo 1  -> 3.3V
Extremo 2  -> GND
Centro     -> PTB1 / ADC0_SE9
```

### Potenciometro 2 - Temperatura

```text
Extremo 1  -> 3.3V
Extremo 2  -> GND
Centro     -> PTB2 / ADC0_SE12
```

### Boton

```text
PTB0 ---- boton ---- 3.3V
```

El pin `PTB0` usa pull-down interno:

```text
0 = no presionado
1 = presionado
```

## Uso de FreeRTOS

Tareas:

- `vTaskLightSensor`
- `vTaskTemperatureSensor`
- `vTaskButtonPolling`
- `vTaskLedControl`
- `vTaskSerialMonitor`

Sincronizacion:

- Ninguna.

Comunicacion:

- Variables globales `volatile`.

## LEDs

```text
Light < 2048        -> LED azul encendido
Temperature > 2048  -> LED rojo encendido
Button = 1          -> LED verde encendido
```

## Pruebas sugeridas

1. Mover los potenciometros y observar los LEDs.
2. Presionar el boton y observar el LED verde.
3. Revisar que `vTaskLedControl` despierta cada 200 ms aunque los datos no cambien.
4. Revisar que el boton usa polling cada 50 ms.

## Liga al video

```text
Pegar aqui la liga del video
```
