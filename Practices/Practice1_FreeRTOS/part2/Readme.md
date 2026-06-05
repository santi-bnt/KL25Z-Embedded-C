# Parte 2 - Safe Data Exchange

## Descripcion

En esta parte se reemplazan las variables globales de la Parte 1 por una **queue de FreeRTOS**.

Cada tarea productora manda un mensaje de tipo `sensor_msg_t` hacia `sensorQueue`, y la tarea `vTaskSystemControl` duerme hasta recibir un dato nuevo.

La aplicacion usa:

* Potenciometro de luz por ADC.
* Potenciometro de temperatura por ADC.
* Boton digital con polling.
* Control de LEDs RGB.
* Envio de datos mediante una queue.
* Monitoreo por terminal serial.

En esta etapa el objetivo principal es mejorar el intercambio de datos entre tareas. Todavia no se protege el ADC con mutex y el boton todavia usa polling. Esos dos puntos se corrigen en la Parte 3.

## Objetivo

La Parte 2 corrige el problema de compartir datos con variables globales. Ahora los datos fluyen usando una queue:

```c
sensorQueue = xQueueCreate(10, sizeof(sensor_msg_t));
```

Las tareas productoras usan:

```c
xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));
```

La tarea consumidora usa:

```c
xQueueReceive(sensorQueue, &msg, portMAX_DELAY);
```

Con esto, la tarea `vTaskSystemControl` ya no necesita despertar constantemente para revisar variables globales. En lugar de eso, permanece bloqueada hasta que llega un nuevo mensaje a la queue.

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

El boton se conecta en `PTA1` con logica invertida:

```text
PTA1 ---- boton ---- GND
```

El pin `PTA1` usa pull-up interno:

```text
Pin fisico = 1 -> no presionado
Pin fisico = 0 -> presionado
```

Dentro del codigo se invierte la lectura para que el sistema trabaje con logica normal:

```text
Button = 0 -> no presionado
Button = 1 -> presionado
```

La inversion se realiza con:

```c
physical_value = (uint16_t)GPIO_ReadPinInput(BUTTON_GPIO, BUTTON_PIN);

msg.type = SENSOR_BUTTON;
msg.value = (uint16_t)!physical_value;
```

## Uso de FreeRTOS

### Tareas

El sistema usa cuatro tareas:

* `vTaskLightSensor`: lee el ADC de luz.
* `vTaskTemperatureSensor`: lee el ADC de temperatura.
* `vTaskButtonPolling`: lee el estado del boton cada 50 ms.
* `vTaskSystemControl`: recibe mensajes, controla LEDs e imprime por serial.

### Sincronizacion

En esta parte se usa solamente:

```c
QueueHandle_t sensorQueue;
```

`sensorQueue` permite enviar datos estructurados desde las tareas productoras hacia la tarea de control.

La estructura usada para los mensajes es:

```c
typedef enum
{
    SENSOR_LIGHT,
    SENSOR_TEMP,
    SENSOR_BUTTON
} sensor_type_t;

typedef struct
{
    sensor_type_t type;
    uint16_t value;
} sensor_msg_t;
```

## Flujo de datos

Cada tarea de sensor crea un mensaje y lo manda a la queue.

Ejemplo para la luz:

```c
msg.type = SENSOR_LIGHT;
msg.value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));
```

Ejemplo para el boton:

```c
physical_value = (uint16_t)GPIO_ReadPinInput(BUTTON_GPIO, BUTTON_PIN);

msg.type = SENSOR_BUTTON;
msg.value = (uint16_t)!physical_value;

xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));
```

La tarea `vTaskSystemControl` espera mensajes con:

```c
xQueueReceive(sensorQueue, &msg, portMAX_DELAY);
```

Esto hace que la tarea consumidora duerma hasta que haya datos disponibles.

## LEDs

La tarea `vTaskSystemControl` recibe mensajes por la queue y controla los LEDs:

```text
Light < 2048        -> LED azul encendido
Temperature > 2048  -> LED rojo encendido
Button = 1          -> LED verde encendido
```

Los valores de luz y temperatura son lecturas crudas del ADC de 12 bits:

```text
Rango ADC: 0 a 4095
```

## Salida esperada

```text
FreeRTOS KL25Z - Parte 2 with Queues
Pot 1 Light -> PTB1 / ADC0_SE9
Pot 2 Temp  -> PTB2 / ADC0_SE12
Button      -> PTA1, inverted logic
Physical button: 1 = not pressed | 0 = pressed
Program button : 0 = not pressed | 1 = pressed

Light: 1870 | Temp: 2450 | Button: 0
Light: 1902 | Temp: 2412 | Button: 1
```

## Pruebas sugeridas

1. Mover el potenciometro conectado a `PTB1` y verificar que cambie el valor de `Light`.
2. Mover el potenciometro conectado a `PTB2` y verificar que cambie el valor de `Temp`.
3. Presionar el boton conectado a `PTA1` y verificar que `Button` cambie de `0` a `1`.
4. Verificar que el LED azul responda al valor de luz.
5. Verificar que el LED rojo responda al valor de temperatura.
6. Verificar que el LED verde se encienda al presionar el boton.
7. Revisar que `vTaskSystemControl` use:

```c
xQueueReceive(sensorQueue, &msg, portMAX_DELAY);
```

8. Confirmar que en esta parte todavia no se usa `xAdcMutex`.
9. Confirmar que el boton todavia usa polling con:

```c
vTaskDelay(pdMS_TO_TICKS(50));
```

## Nota sobre la Parte 3

En esta Parte 2 todavia existen dos limitaciones:

1. Las tareas de luz y temperatura comparten `ADC0` sin mutex.
2. El boton todavia se revisa mediante polling.

Estas limitaciones son intencionales para esta etapa. En la Parte 3 se corrigen usando:

* `xAdcMutex` para proteger el ADC.
* Interrupcion de hardware para el boton.
* `xButtonSemaphore` para despertar la tarea del boton desde la ISR.

## Liga al video

```text
https://drive.google.com/file/d/1G7ptXty3rhMK60_xGqo68oNOcbZGnvdK/view?usp=sharing
```
