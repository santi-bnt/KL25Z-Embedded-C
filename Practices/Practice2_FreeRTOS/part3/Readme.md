# Parte 3 - Event-Driven Architecture

## Descripcion

En esta parte se implementa la arquitectura Event-Driven de FreeRTOS usando mutexes e interrupciones.

La aplicacion mantiene el mismo comportamiento externo de la Parte 2:

- Potenciometro de luz por ADC.
- Potenciometro de temperatura por ADC.
- Boton digital.
- Control de LEDs RGB.
- Envio de datos mediante una queue.
- Monitoreo por terminal serial.

La diferencia importante esta en la arquitectura interna:

- El ADC0 queda protegido con un mutex llamado `xAdcMutex`.
- El boton deja de usar polling.
- El boton despierta una tarea mediante interrupcion de hardware y un semaforo binario.

## Objetivo de la parte 3

Stage 3 corrige dos problemas de la Parte 2:

1. Dos tareas podian acceder al ADC0 al mismo tiempo.
2. La tarea del boton gastaba CPU revisando el pin cada cierto tiempo.

Para resolverlo se usan:

- `xSemaphoreCreateMutex()` para crear el mutex del ADC.
- `xSemaphoreTake()` y `xSemaphoreGive()` para proteger cada lectura del ADC.
- `xSemaphoreCreateBinary()` para crear el semaforo del boton.
- `PORTB_IRQHandler()` para atender la interrupcion del boton.
- `xSemaphoreGiveFromISR()` para despertar la tarea del boton desde la ISR.
- `xSemaphoreTake(xButtonSemaphore, portMAX_DELAY)` para que la tarea del boton duerma hasta que haya un evento real.

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

Como el boton esta conectado en `PTB0`, la interrupcion se atiende con:

```c
void PORTB_IRQHandler(void)
```

## Uso de FreeRTOS

### Tareas

El sistema usa cuatro tareas:

- `vTaskLightSensor`: lee el ADC de luz.
- `vTaskTemperatureSensor`: lee el ADC de temperatura.
- `vTaskButtonInterrupt`: espera el semaforo del boton.
- `vTaskSystemControl`: recibe mensajes, controla LEDs e imprime por serial.

### Sincronizacion

```c
QueueHandle_t sensorQueue;
SemaphoreHandle_t xAdcMutex;
SemaphoreHandle_t xButtonSemaphore;
```

`sensorQueue` transporta mensajes de sensores hacia la tarea de control.

`xAdcMutex` protege el periferico compartido `ADC0`.

`xButtonSemaphore` sincroniza la interrupcion del boton con la tarea que procesa el evento.

## Proteccion del ADC con Mutex

Las dos tareas de sensores analogicos usan el mismo ADC0. Por eso cada lectura queda protegida asi:

```c
if(xSemaphoreTake(xAdcMutex, portMAX_DELAY) == pdTRUE)
{
    ADC16_SetChannelConfig(ADC_BASE, 0U, &adcConfigLight);

    while(0U == (kADC16_ChannelConversionDoneFlag &
                 ADC16_GetChannelStatusFlags(ADC_BASE, 0U)))
    {
    }

    msg.value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

    xSemaphoreGive(xAdcMutex);
}
```

Esto evita que una tarea cambie la configuracion del canal ADC mientras otra todavia esta haciendo una conversion.

## Boton con interrupcion

El boton se configura para generar interrupcion por cambio de flanco:

```c
PORT_SetPinInterruptConfig(BUTTON_PORT, BUTTON_PIN, kPORT_InterruptEitherEdge);
EnableIRQ(PORTB_IRQn);
```

La ISR solamente limpia la bandera y libera el semaforo:

```c
void PORTB_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    PORT_ClearPinsInterruptFlags(BUTTON_PORT, (1U << BUTTON_PIN));
    xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

La tarea del boton ya no hace polling. Duerme hasta que la ISR la despierta:

```c
xSemaphoreTake(xButtonSemaphore, portMAX_DELAY);
```

Despues aplica debounce de 50 ms, lee el pin, manda el mensaje a la queue y limpia eventos extra causados por rebotes.

## LEDs

La tarea `vTaskSystemControl` recibe mensajes por la queue y controla los LEDs:

```text
Light < 2048        -> LED azul encendido
Temperature > 2048  -> LED rojo encendido
Button = 1          -> LED verde encendido
```

## Salida esperada

```text
FreeRTOS KL25Z - Parte 3 Event Driven
ADC protected with xAdcMutex
Button PTB0 wakes task using ISR + binary semaphore
Pot 1 Light -> PTB1 / ADC0_SE9
Pot 2 Temp  -> PTB2 / ADC0_SE12
Button logic: 0 = not pressed | 1 = pressed

Light: 1870 | Temp: 2450 | Button: 0
Light: 1902 | Temp: 2412 | Button: 1
```

## Pruebas sugeridas

1. Mover ambos potenciometros y verificar que los LEDs azul y rojo respondan.
2. Revisar en el codigo que las llamadas a `ADC16_SetChannelConfig()` y `ADC16_GetChannelConversionValue()` esten dentro del bloque protegido por `xAdcMutex`.
3. Presionar el boton y verificar que el LED verde cambie sin usar polling.
4. Revisar que `vTaskButtonInterrupt` este bloqueada con `xSemaphoreTake(xButtonSemaphore, portMAX_DELAY)`.

## Liga al video

```text
Pegar aqui la liga del video
```
