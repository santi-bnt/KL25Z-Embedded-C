# Parte 3 - Event-Driven Architecture

## Descripcion

En esta parte se implementa la arquitectura **Event-Driven** de FreeRTOS usando **mutexes, interrupciones y semaforos binarios**.

La aplicacion mantiene el mismo comportamiento externo de la Parte 2:

* Sensor de luz por ADC.
* Sensor analogico de temperatura por ADC.
* Boton digital.
* Control de LEDs RGB.
* Envio de datos mediante una queue.
* Monitoreo por terminal serial.

La diferencia importante esta en la arquitectura interna:

* El ADC0 queda protegido con un mutex llamado `xAdcMutex`.
* El boton deja de usar polling.
* El boton despierta una tarea mediante una interrupcion de hardware y un semaforo binario.
* Las lecturas de los sensores se envian a una tarea de control mediante `sensorQueue`.

## Objetivo de la parte 3

Stage 3 corrige dos problemas de la Parte 2:

1. Dos tareas podian acceder al ADC0 al mismo tiempo.
2. La tarea del boton podia gastar CPU revisando el pin cada cierto tiempo.

Para resolverlo se usan:

* `xSemaphoreCreateMutex()` para crear el mutex del ADC.
* `xSemaphoreTake()` y `xSemaphoreGive()` para proteger cada lectura del ADC.
* `xSemaphoreCreateBinary()` para crear el semaforo del boton.
* `PORTA_IRQHandler()` para atender la interrupcion del boton.
* `xSemaphoreGiveFromISR()` para despertar la tarea del boton desde la ISR.
* `xSemaphoreTake(xButtonSemaphore, portMAX_DELAY)` para que la tarea del boton duerma hasta que haya un evento real.

## Conexiones

### Sensor de luz

La fotoresistencia se conecta como divisor de voltaje hacia una entrada analogica.

```text
3.3V ---- Fotoresistencia ---- PTB1 / ADC0_SE9 ---- Resistencia 10kΩ ---- GND
```

Si se usa un potenciometro para simular luz, la conexion es:

```text
Extremo 1  -> 3.3V
Extremo 2  -> GND
Centro     -> PTB1 / ADC0_SE9
```

### Sensor analogico de temperatura

Para la entrada de temperatura se usa un sensor analogico o un potenciometro conectado al ADC.

```text
Extremo 1  -> 3.3V
Extremo 2  -> GND
Centro     -> PTB2 / ADC0_SE12
```

En esta version se reporta el valor crudo del ADC, no se inventan grados Celsius ni humedad.

### Boton

El boton se conecta con logica invertida usando pull-up interno:

```text
PTA1 ---- boton ---- GND
```

El pin `PTA1` usa pull-up interno:

```text
Pin fisico = 1 -> no presionado
Pin fisico = 0 -> presionado
```

Dentro del codigo se invierte la lectura para que el sistema trabaje de forma normal:

```text
Button = 0 -> no presionado
Button = 1 -> presionado
```

Como el boton esta conectado en `PTA1`, la interrupcion se atiende con:

```c
void PORTA_IRQHandler(void)
```

## Uso de FreeRTOS

### Tareas

El sistema usa cuatro tareas:

* `vTaskLightSensor`: lee el ADC de luz.
* `vTaskTemperatureSensor`: lee el ADC de temperatura.
* `vTaskButtonPolling`: espera el semaforo del boton.
* `vTaskSystemControl`: recibe mensajes, controla LEDs e imprime por serial.

Aunque la tarea se llama `vTaskButtonPolling`, no realiza polling. La tarea permanece bloqueada hasta que la interrupcion del boton libera el semaforo binario.

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

    msg.type = SENSOR_LIGHT;
    msg.value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

    xSemaphoreGive(xAdcMutex);

    xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));
}
```

Esto evita que una tarea cambie la configuracion del canal ADC mientras otra todavia esta haciendo una conversion.

## Boton con interrupcion

El boton se configura para generar interrupcion por cambio de flanco:

```c
PORT_SetPinInterruptConfig(BUTTON_PORT, BUTTON_PIN, kPORT_InterruptEitherEdge);

NVIC_SetPriority(PORTA_IRQn, 3U);
EnableIRQ(PORTA_IRQn);
```

La ISR solamente limpia la bandera y libera el semaforo:

```c
void PORTA_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    PORT_ClearPinsInterruptFlags(BUTTON_PORT, (1U << BUTTON_PIN));

    if(xButtonSemaphore != NULL)
    {
        xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

La tarea del boton ya no revisa constantemente el pin. Duerme hasta que la ISR la despierta:

```c
xSemaphoreTake(xButtonSemaphore, portMAX_DELAY);
```

Despues aplica debounce de 50 ms, lee el pin, invierte la logica del boton, manda el mensaje a la queue y limpia eventos extra causados por rebotes.

```c
vTaskDelay(pdMS_TO_TICKS(50));

physical_value = (uint16_t)GPIO_ReadPinInput(BUTTON_GPIO, BUTTON_PIN);

msg.type = SENSOR_BUTTON;
msg.value = (uint16_t)!physical_value;

xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));

while(xSemaphoreTake(xButtonSemaphore, 0) == pdTRUE)
{
}
```

Con esto la tarea del boton trabaja con 0% de CPU mientras no se presiona el boton.

## LEDs

La tarea `vTaskSystemControl` recibe mensajes por la queue y controla los LEDs:

```text
Light ADC < 2048  -> LED azul encendido
Temp ADC > 2048   -> LED rojo encendido
Button = 1        -> LED verde encendido
```

Los valores de luz y temperatura son lecturas crudas del ADC de 12 bits:

```text
Rango ADC: 0 a 4095
```

## Salida esperada

```text
FreeRTOS KL25Z - Stage 3 Event Driven
ADC0 protected with xAdcMutex
Button uses PORTA interrupt + binary semaphore
Light sensor -> PTB1 / ADC0_SE9
Temperature analog input -> PTB2 / ADC0_SE12
Button -> PTA1 / PORTA IRQ, inverted logic

Light ADC: 1870 | Temp ADC: 2450 | Button: 0
Light ADC: 1902 | Temp ADC: 2412 | Button: 1
```

## Pruebas sugeridas

1. Mover o cubrir la fotoresistencia y verificar que cambia el valor de `Light ADC`.
2. Mover el potenciometro o variar el sensor analogico de temperatura y verificar que cambia el valor de `Temp ADC`.
3. Revisar que el LED azul responda al umbral de luz.
4. Revisar que el LED rojo responda al umbral de temperatura analogica.
5. Revisar en el codigo que las llamadas a `ADC16_SetChannelConfig()` y `ADC16_GetChannelConversionValue()` esten dentro del bloque protegido por `xAdcMutex`.
6. Presionar el boton conectado a `PTA1` y verificar que el LED verde cambie.
7. Revisar que `vTaskButtonPolling` este bloqueada con:

```c
xSemaphoreTake(xButtonSemaphore, portMAX_DELAY)
```

8. Confirmar que la interrupcion del boton use:

```c
PORTA_IRQHandler()
```

9. Confirmar que dentro de la ISR se llame:

```c
xSemaphoreGiveFromISR()
```

## Liga al video

```text
https://drive.google.com/file/d/16CxJyuVRiDLHzSFemQGUbMAWlAAhxugr/view?usp=sharing
```
