# Parte 3 - Lectura de Luminosidad y Temperatura

## Descripción de la parte 3

En esta parte se modificaron las tareas `vLightSensor` y `vTempSensor` para leer datos de luminosidad y temperatura. El sistema utiliza una entrada analógica para la luminosidad y un sensor DHT11 para temperatura y humedad.

La instrucción menciona usar una fórmula similar a:

```c
temperature = result * 330.0 / 65536;
```

Sin embargo, esa fórmula aplica para sensores analógicos conectados al ADC, como el LM35. En este caso se utilizó un DHT11, el cual no entrega una señal analógica, sino datos digitales por GPIO. Por eso, la fórmula del ADC no se aplicó directamente a la temperatura.

La fórmula sí se aplicó a la lectura analógica de luminosidad, ajustando la resolución del ADC configurado a 12 bits.

---

## Conversión del ADC

La fórmula de la diapositiva usa `65536`, lo cual corresponde a un ADC de 16 bits:

```text
2^16 = 65536
```

En este proyecto el ADC está configurado a 12 bits, por lo que se usa:

```text
2^12 = 4096
```

Para convertir la lectura de luminosidad a voltaje se usó:

```c
light_voltage_mv = result * 3300 / 4096;
```

Donde:

```text
result = lectura del ADC
3300 = voltaje de referencia en milivolts
4096 = resolución del ADC de 12 bits
```

---

## Sensor de luminosidad

La luminosidad se lee mediante el ADC en el pin:

```text
PTB1 / ADC0_SE9
```

Conexión:

```text
Entrada de luminosidad:

Extremo 1  -> 3.3V
Extremo 2  -> GND
Centro     -> PTB1 / ADC0_SE9
```

Para definir el threshold se tomaron valores mínimo y máximo:

```text
Valor mínimo: PEGAR_VALOR_AQUI
Valor máximo: PEGAR_VALOR_AQUI
Threshold elegido: PEGAR_VALOR_AQUI
```

El threshold permite clasificar la lectura como baja u alta luminosidad.

```c
if(light_raw < LIGHT_THRESHOLD)
{
    PRINTF("Light status: DARK\r\n");
}
else
{
    PRINTF("Light status: BRIGHT\r\n");
}
```

---

## Sensor de temperatura y humedad DHT11

Para temperatura se utilizó un DHT11. Este sensor mide temperatura y humedad, pero no se lee por ADC. Su lectura se realiza mediante comunicación digital por un pin GPIO.

Conexión:

```text
DHT11:

VCC  -> 3.3V
GND  -> GND
DATA -> PTC2
```

También se recomienda usar una resistencia pull-up:

```text
3.3V ---- 10 kΩ ---- DATA / PTC2
```

El DHT11 entrega directamente:

```text
Temperatura en °C
Humedad relativa en %
```

Por eso, en esta parte no se usó la fórmula del ADC para temperatura. La temperatura se obtiene directamente del dato digital enviado por el DHT11.

---

## Botón

También se utilizó un botón como entrada digital.

```text
PTB0 ---- botón ---- 3.3V
```

Lógica usada:

```text
0 = no presionado
1 = presionado
```

---

## Tareas modificadas

### `vLightSensor`

Esta tarea lee el valor analógico de luminosidad:

```c
result = ADC0_ReadChannel(ADC_CH_LIGHT);
```

Después, el valor se envía por queue:

```c
msg.type = SENSOR_LIGHT;
msg.value = result;
xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));
```

---

### `vTempSensor`

Esta tarea lee el DHT11 y obtiene temperatura y humedad:

```c
if(DHT11_Read(&temperature, &humidity))
{
    msgTemp.type = SENSOR_TEMP;
    msgTemp.value = temperature;
    xQueueSend(sensorQueue, &msgTemp, pdMS_TO_TICKS(10));

    msgHumidity.type = SENSOR_HUMIDITY;
    msgHumidity.value = humidity;
    xQueueSend(sensorQueue, &msgHumidity, pdMS_TO_TICKS(10));
}
```

---

## Salida esperada en terminal

```text
Light raw: 350 | Light voltage: 281 mV | Light: 8 % | Temp DHT11: 25 C | Humidity: 50 % | Button: 0
Light status: DARK
Temperature status: NORMAL
Button status: NOT PRESSED | Button value: 0
```

---

## Liga al video

```text
https://drive.google.com/file/d/1p4wGMJ25Cfc5pDbM9Z84csHmh2bGohwo/view?usp=sharing
```

---

## Conclusión

En esta parte se modificaron las tareas para leer luminosidad y temperatura. La luminosidad se leyó mediante ADC, por lo que se aplicó una fórmula de conversión ajustada a 12 bits usando `4096`. Para temperatura se utilizó el DHT11, que entrega datos digitales de temperatura y humedad, por lo que no se aplicó directamente la fórmula del ADC a ese sensor.