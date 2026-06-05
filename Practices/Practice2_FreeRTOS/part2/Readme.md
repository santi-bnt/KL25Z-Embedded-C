# Parte 2 - Safe Data Exchange

## Descripcion

En esta parte se reemplazan las variables globales de la Parte 1 por una queue de FreeRTOS.

Cada tarea productora manda un mensaje de tipo `sensor_msg_t` hacia `sensorQueue`, y la tarea `vTaskSystemControl` duerme hasta recibir un dato nuevo.

## Objetivo

La Parte 2 corrige el problema de compartir datos con variables globales. Ahora los datos fluyen usando:

```c
sensorQueue = xQueueCreate(10, sizeof(sensor_msg_t));
```

Los productores usan:

```c
xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));
```

La tarea consumidora usa:

```c
xQueueReceive(sensorQueue, &msg, portMAX_DELAY);
```

Esto evita que la tarea de control despierte sin necesidad.

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
- `vTaskSystemControl`

Sincronizacion:

- `sensorQueue`

En esta parte todavia no se usa mutex para el ADC y el boton todavia usa polling. Esos dos problemas se corrigen en la Parte 3.

## LEDs

```text
Light < 2048        -> LED azul encendido
Temperature > 2048  -> LED rojo encendido
Button = 1          -> LED verde encendido
```

## Liga al video

```text
Pegar aqui la liga del video
```
