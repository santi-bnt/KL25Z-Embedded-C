# Parte 1 - Basic Concurrency

## Descripcion

En esta parte se implementa una aplicacion basica con FreeRTOS usando varias tareas concurrentes.

La aplicacion utiliza:

* Potenciometro de luz por ADC.
* Potenciometro de temperatura por ADC.
* Boton digital con polling.
* Variables globales compartidas.
* Control de LEDs RGB.
* Monitoreo por terminal serial.

Esta primera parte sirve como base para observar un sistema funcional, pero todavia con una arquitectura simple. Se usan variables globales para compartir datos entre tareas y el boton se revisa constantemente mediante polling.

En esta etapa todavia no se usan queues, mutexes ni interrupciones.

## Objetivo

El objetivo de la Parte 1 es entender como funcionan varias tareas ejecutandose con FreeRTOS.

El sistema se divide en tareas independientes:

* Una tarea lee el sensor de luz.
* Una tarea lee el sensor de temperatura.
* Una tarea revisa el boton.
* Una tarea controla los LEDs.
* Una tarea imprime los valores por terminal serial.

Los datos se comparten mediante variables globales:

```c
volatile uint16_t light_value = 0;
volatile uint16_t temp_value = 0;
volatile uint8_t button_state = 0;
```

Esta forma funciona para una primera version, pero tiene limitaciones. Las tareas pueden leer datos mientras otras los estan modificando, y algunas tareas despiertan aunque no haya datos nuevos.

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

Dentro del codigo se invierte la lectura para trabajar con logica normal:

```text
Button = 0 -> no presionado
Button = 1 -> presionado
```

La inversion se realiza con:

```c
physical_value = (uint8_t)GPIO_ReadPinInput(BUTTON_GPIO, BUTTON_PIN);
button_state = (uint8_t)!physical_value;
```

## Uso de FreeRTOS

### Tareas

El sistema usa cinco tareas:

* `vTaskLightSensor`: lee el ADC de luz.
* `vTaskTemperatureSensor`: lee el ADC de temperatura.
* `vTaskButtonPolling`: revisa el boton cada 50 ms.
* `vTaskLedControl`: controla los LEDs usando las variables globales.
* `vTaskSerialMonitor`: imprime los valores por terminal serial.

### Sincronizacion

En esta parte no se usa ningun mecanismo de sincronizacion avanzado.

No se usan:

* Queues.
* Mutexes.
* Semaforos.
* Interrupciones.

La comunicacion entre tareas se hace por variables globales.

## Lectura de sensores

La tarea de luz lee el canal `ADC0_SE9`:

```c
ADC16_SetChannelConfig(ADC_BASE, 0U, &adcConfigLight);

while(0U == (kADC16_ChannelConversionDoneFlag &
             ADC16_GetChannelStatusFlags(ADC_BASE, 0U)))
{
}

light_value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);
```

La tarea de temperatura lee el canal `ADC0_SE12`:

```c
ADC16_SetChannelConfig(ADC_BASE, 0U, &adcConfigTemperature);

while(0U == (kADC16_ChannelConversionDoneFlag &
             ADC16_GetChannelStatusFlags(ADC_BASE, 0U)))
{
}

temp_value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);
```

Los valores de luz y temperatura son lecturas crudas del ADC de 12 bits:

```text
Rango ADC: 0 a 4095
```

## Boton con polling

En esta Parte 1 el boton se revisa cada 50 ms:

```c
physical_value = (uint8_t)GPIO_ReadPinInput(BUTTON_GPIO, BUTTON_PIN);
button_state = (uint8_t)!physical_value;

vTaskDelay(pdMS_TO_TICKS(50));
```

Esto significa que la tarea despierta constantemente para revisar el estado del pin, aunque el usuario no haya presionado el boton.

Esta limitacion se corrige en la Parte 3 usando interrupciones.

## LEDs

La tarea `vTaskLedControl` usa las variables globales para controlar los LEDs:

```text
Light < 2048        -> LED azul encendido
Temperature > 2048  -> LED rojo encendido
Button = 1          -> LED verde encendido
```

## Salida esperada

```text
FreeRTOS KL25Z - Parte 1 Basic Concurrency
Global variables + polling
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
7. Confirmar que en esta parte se usan variables globales y no queues.
8. Confirmar que el boton todavia usa polling con:

```c
vTaskDelay(pdMS_TO_TICKS(50));
```

## Limitaciones de esta parte

Esta Parte 1 funciona, pero tiene limitaciones importantes:

1. Se usan variables globales para compartir datos entre tareas.
2. El boton usa polling y revisa el pin constantemente.
3. El ADC0 es usado por dos tareas sin proteccion.
4. La tarea de LEDs despierta cada cierto tiempo aunque no necesariamente haya datos nuevos.

Estas limitaciones son intencionales para esta etapa. En la Parte 2 se reemplazan las variables globales por una queue, y en la Parte 3 se agregan mutexes e interrupciones.

## Liga al video

```text
https://drive.google.com/file/d/1r3tFSQxEbtizOQSsYtAGQ4lo3t3vvj1e/view?usp=sharing
```
