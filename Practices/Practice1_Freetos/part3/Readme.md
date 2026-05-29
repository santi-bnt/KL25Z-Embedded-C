# Parte 3 - Lectura de sensores de luminosidad y temperatura

## Descripción

En esta parte de la práctica se modificaron las tareas `vLightSensor` y `vTempSensor` para leer datos provenientes de sensores físicos. El objetivo fue utilizar el ADC de la KL25Z para obtener valores analógicos y convertirlos a unidades útiles mediante una fórmula basada en la resolución configurada del ADC.

El sistema trabaja con:

* Sensor de luminosidad mediante LDR.
* Sensor de temperatura analógico, recomendado LM35.
* ADC configurado a 12 bits.
* FreeRTOS para ejecutar tareas independientes.
* Terminal serial para visualizar los datos obtenidos.

---

## Objetivo

Modificar las tareas de lectura para que obtengan datos reales de sensores:

* `vLightSensor`: leer el sensor de luminosidad.
* `vTempSensor`: leer el sensor de temperatura.

Además, se debe aplicar una fórmula de conversión similar a la vista en clase:

```c
temperature = result * 330.0 / 65536;
```

Sin embargo, el divisor debe ajustarse de acuerdo con la resolución configurada del ADC.

---

## Resolución del ADC

En esta práctica el ADC se configuró a 12 bits. Esto significa que la lectura del ADC puede tomar valores desde 0 hasta 4095.

```text
2^12 = 4096
```

Por lo tanto, en lugar de usar `65536`, que corresponde a un ADC de 16 bits, se usa `4096`.

La fórmula general es:

```c
valor_convertido = result * escala / resolucion;
```

Para un ADC de 12 bits:

```c
valor_convertido = result * escala / 4096;
```

---

## Sensor de temperatura LM35

El LM35 es un sensor analógico de temperatura. Su salida cambia linealmente con la temperatura.

La relación del LM35 es:

```text
10 mV = 1 °C
```

Como la KL25Z trabaja con una referencia de 3.3 V, se puede convertir la lectura del ADC a temperatura usando:

```c
temperature = result * 330.0 / 4096.0;
```

Donde:

* `result` es la lectura del ADC.
* `330.0` representa 3.3 V convertidos a grados Celsius, considerando que el LM35 entrega 10 mV por cada °C.
* `4096.0` corresponde a la resolución del ADC de 12 bits.

También puede expresarse en dos pasos:

```c
voltage = result * 3.3 / 4096.0;
temperature = voltage * 100.0;
```

Ambas formas son equivalentes.

---

## Conexión del LM35

```text
LM35        KL25Z
VCC   --->  3.3V
GND   --->  GND
VOUT  --->  Pin ADC para temperatura
```

Ejemplo de conexión:

```text
LM35 VOUT ---> PTB2 / ADC0_SE12
```

---

## Sensor de luminosidad LDR

Para medir luminosidad se utiliza una LDR conectada como divisor de voltaje junto con una resistencia de 10 kΩ.

La conexión recomendada es:

```text
3.3V
 |
[LDR]
 |
 |------ Pin ADC de luz
 |
[10 kΩ]
 |
GND
```

Ejemplo:

```text
Punto medio del divisor ---> PTB1 / ADC0_SE9
```

La lectura del ADC representa el nivel de luminosidad. Para obtener el voltaje aproximado se puede usar:

```c
light_voltage = result * 3.3 / 4096.0;
```

O en milivolts:

```c
light_voltage_mv = result * 3300 / 4096;
```

---

## Calibración del sensor de luz

Para definir el threshold del sensor de luz, primero se deben medir dos valores:

```text
Valor mínimo: lectura cuando el sensor está oscuro.
Valor máximo: lectura cuando el sensor está iluminado.
```

Ejemplo:

```text
Valor en oscuridad: 300
Valor con luz: 3000
```

Con esos valores se elige un threshold intermedio:

```text
Threshold elegido: 1800
```

Después se puede clasificar el estado de luz:

```c
if(light_value < LIGHT_THRESHOLD)
{
    PRINTF("Light status: DARK\r\n");
}
else
{
    PRINTF("Light status: BRIGHT\r\n");
}
```

El valor exacto del threshold depende de las mediciones reales obtenidas durante la prueba.

---

## Tareas modificadas

### `vLightSensor`

Esta tarea se encarga de leer el valor analógico del sensor de luminosidad mediante el ADC. Después, envía el valor leído para que pueda ser mostrado en la terminal o usado por otra tarea.

Ejemplo de conversión:

```c
light_voltage_mv = result * 3300 / 4096;
```

### `vTempSensor`

Esta tarea se encarga de leer el valor analógico del sensor de temperatura. Si se usa LM35, la lectura del ADC se convierte a grados Celsius con:

```c
temperature = result * 330.0 / 4096.0;
```

Esto permite mostrar la temperatura real en °C en la terminal serial.

---

## Salida esperada en terminal

Un ejemplo de salida esperada es:

```text
Light raw: 300 | Light voltage: 241 mV | Light status: DARK
Temperature raw: 320 | Temperature: 25.78 C

Light raw: 3000 | Light voltage: 2416 mV | Light status: BRIGHT
Temperature raw: 335 | Temperature: 26.98 C
```

---

## Nota sobre el DHT11

Aunque inicialmente se consideró usar DHT11, este sensor no entrega una señal analógica. El DHT11 funciona mediante comunicación digital por un pin GPIO, por lo que no utiliza directamente la fórmula de conversión del ADC.

Por esta razón, para cumplir mejor con la instrucción de aplicar una fórmula basada en la lectura del ADC, es más adecuado usar un sensor analógico como el LM35.

---

## Conclusión

En esta parte se implementó la lectura de sensores usando el ADC de la KL25Z. Para que las conversiones fueran correctas, se tomó en cuenta que el ADC estaba configurado a 12 bits, por lo que se utilizó `4096` como resolución. El sensor de luz se calibró obteniendo valores mínimo y máximo para definir un threshold, mientras que el sensor de temperatura LM35 permitió convertir directamente la lectura analógica a grados Celsius.
