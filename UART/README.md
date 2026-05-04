# Mini Radar System with KL25Z

## Integrantes

Joshua Menchaca - Santiago Benavent - Jared García - André Pinto

---

## Descripción

Este proyecto implementa un sistema tipo radar usando una **KL25Z**, un **sensor ultrasónico HC-SR04**, un **motor paso a paso** y comunicación **UART** hacia una computadora.

El sistema realiza un barrido angular, mide distancias y envía los datos a una computadora para visualizarlos en tiempo real. La KL25Z controla el movimiento del motor, toma la lectura del sensor ultrasónico y transmite la información usando UART.

El formato enviado a Python es:

```txt
ANGULO,DISTANCIA
```

Ejemplo:

```txt
90,45
```

Python recibe los datos con **PySerial** y los grafica en tiempo real con **Matplotlib**, generando una visualización tipo radar en 2D.

---

## Materiales

- KL25Z
- Sensor ultrasónico HC-SR04
- Motor paso a paso
- Driver L293D o ULN2003
- Fuente externa de 5V
- Protoboard
- Cables jumper
- Computadora con Python
- Comunicación UART hacia PC

---

## Funcionamiento general

```txt
1. La KL25Z inicializa UART, GPIO, sensor ultrasónico y motor.
2. El motor paso a paso realiza un barrido de 0° a 180°.
3. Al llegar al límite, el motor regresa de 180° a 0°.
4. Cada cierto número de pasos, la KL25Z mide la distancia con el HC-SR04.
5. La KL25Z calcula el ángulo actual.
6. Se asocia cada distancia con su ángulo correspondiente.
7. La KL25Z envía ANGULO,DISTANCIA por UART.
8. Python recibe los datos y genera la gráfica tipo radar.
```

---

## Conceptos principales

Este proyecto integra varios conceptos de sistemas embebidos:

- Control de GPIO
- Secuencia de motor paso a paso
- Medición de distancia con sensor ultrasónico
- Control de tiempos mediante retardos
- Comunicación serial UART
- Formateo de datos con `sprintf`
- Visualización en tiempo real con Python
- Integración de hardware y software

---

# Parte 1: Comunicación UART

## Descripción

La comunicación UART se utiliza para enviar los datos desde la KL25Z hacia la computadora. En este proyecto, la KL25Z solamente transmite información, por lo que se usa el modo **TX**.

---

## Configuración UART

```txt
Baud rate: 57600
Formato enviado: ANGULO,DISTANCIA
Ejemplo: 90,45
```

Ejemplos de datos enviados:

```txt
45,120
90,85
135,200
```

---

## Flujo de UART

```txt
1. Se habilita el reloj de UART0.
2. Se configura PTA2 como UART0_TX.
3. Se establece el baud rate en 57600.
4. Los valores de ángulo y distancia se convierten a texto.
5. La cadena se envía carácter por carácter a la computadora.
```

---

# Parte 2: Control del motor paso a paso

## Descripción

El motor paso a paso se encarga de mover el sensor ultrasónico para generar el barrido angular del radar.

El motor se controla mediante cuatro señales digitales conectadas al driver L293D o ULN2003. La KL25Z activa las bobinas del motor siguiendo una secuencia definida.

---

## Comportamiento del motor

```txt
1. El motor avanza paso por paso.
2. Se incrementa un contador interno de pasos.
3. El sistema calcula el ángulo actual.
4. El motor avanza desde 0° hasta 180°.
5. Al llegar a 180°, cambia de dirección.
6. El motor regresa de 180° hasta 0°.
7. El ciclo se repite continuamente.
```

---

## Secuencia de pasos

Secuencia básica usada para activar las bobinas:

```txt
0x01 -> 0x02 -> 0x04 -> 0x08
```

Esta secuencia permite controlar el movimiento del motor paso a paso.

---

## Fórmula para el ángulo

```txt
angulo = (pasos_totales * 180) / 1024
```

Donde:

```txt
pasos_totales = contador de pasos actual
180 = rango máximo del radar en grados
1024 = número de pasos aproximados para cubrir 180°
```

---

# Parte 3: Sensor ultrasónico HC-SR04

## Descripción

El sensor ultrasónico HC-SR04 se utiliza para medir la distancia entre el radar y un objeto.

El sensor funciona enviando un pulso ultrasónico y midiendo el tiempo que tarda en regresar el eco.

---

## Funcionamiento del HC-SR04

```txt
1. La KL25Z coloca el pin TRIG en alto durante 10 microsegundos.
2. El sensor emite una onda ultrasónica.
3. El pin ECHO cambia a alto mientras espera el rebote de la señal.
4. La KL25Z mide cuánto tiempo permanece ECHO en alto.
5. Con ese tiempo se calcula la distancia.
```

---

## Fórmula de distancia

```txt
distancia = (tiempo * 0.0343) / 2
```

Donde:

```txt
tiempo = duración del pulso ECHO
0.0343 = velocidad aproximada del sonido en cm/us
2 = se divide entre 2 porque el sonido viaja de ida y vuelta
```

---

## Nota sobre ECHO

El pin **ECHO** del HC-SR04 puede entregar una señal de **5V**, mientras que la KL25Z trabaja con lógica de **3.3V**.

Por seguridad, se recomienda usar un divisor de voltaje antes de conectar ECHO a la KL25Z.

---

# Parte 4: Integración del sistema

## Descripción

En esta parte se combinan el motor paso a paso, el sensor ultrasónico y la comunicación UART para crear el sistema tipo radar.

La KL25Z mueve el motor, mide la distancia y envía la información a la computadora.

---

## Flujo principal del sistema

```txt
Inicialización:
1. Inicializar UART.
2. Inicializar pines GPIO.
3. Inicializar sensor ultrasónico.
4. Inicializar motor paso a paso.

Ciclo principal:
1. Mover el motor un paso.
2. Actualizar el contador de pasos.
3. Calcular el ángulo.
4. Cada 4 pasos, medir distancia.
5. Enviar ángulo y distancia por UART.
6. Repetir el proceso.
```

---

## Razón de medir cada 4 pasos

La distancia no se mide en cada paso para evitar que el sistema sea demasiado lento.

Medir cada 4 pasos permite mantener un equilibrio entre:

- Velocidad de barrido
- Estabilidad de lectura
- Cantidad de datos enviados
- Tiempo de procesamiento

---

# Conexiones

## Sensor ultrasónico HC-SR04

| HC-SR04 | KL25Z | Función |
|---|---|---|
| VCC | 5V | Alimentación |
| GND | GND | Tierra común |
| TRIG | PTB0 | Disparo del sensor |
| ECHO | PTB1 | Lectura del eco |

> Importante: el pin ECHO puede entregar 5V. Se recomienda usar un divisor de voltaje para bajarlo a 3.3V antes de conectarlo a PTB1.

---

## Motor paso a paso con driver

| Driver | KL25Z | Función |
|---|---|---|
| IN1 | PTD0 | Bobina 1 |
| IN2 | PTD1 | Bobina 2 |
| IN3 | PTD2 | Bobina 3 |
| IN4 | PTD3 | Bobina 4 |

---

## Conexiones de alimentación del L293D

| Pin del L293D | Conexión |
|---|---|
| VCC1 | 5V lógica |
| VCC2 | Fuente externa para motor |
| GND | Tierra común |

> El motor no debe alimentarse directamente desde la KL25Z. Debe usarse una fuente externa y compartir GND con la KL25Z.

---

## UART

| Señal | KL25Z | Función |
|---|---|---|
| TX | PTA2 | Envío de datos a la computadora |
| GND | GND | Tierra común |

La KL25Z solamente transmite datos hacia la computadora usando **UART0_TX en PTA2**.

---

## Pines usados

```txt
PTB0 -> TRIG
PTB1 -> ECHO
PTD0 -> IN1
PTD1 -> IN2
PTD2 -> IN3
PTD3 -> IN4
PTA2 -> UART0_TX
GND  -> Tierra común
```

---

## Diagrama general de conexiones

```txt
KL25Z
│
├── Sensor ultrasónico HC-SR04
│   ├── TRIG -> PTB0
│   └── ECHO -> PTB1
│
├── Driver L293D o ULN2003
│   ├── IN1 -> PTD0
│   ├── IN2 -> PTD1
│   ├── IN3 -> PTD2
│   └── IN4 -> PTD3
│
└── UART
    └── TX -> PTA2 -> Computadora
```

---

# Comunicación UART

## Configuración

```txt
Baud rate: 57600
Formato enviado: ANGULO,DISTANCIA
Ejemplo: 90,45
```

---

## Formato de datos

Los datos se envían como texto, separados por coma:

```txt
ANGULO,DISTANCIA
```

Ejemplos:

```txt
0,120
45,80
90,35
135,100
180,150
```

Python lee cada línea, separa el ángulo y la distancia, y los usa para dibujar el radar.

---

# Visualización en Python

## Descripción

La computadora recibe los datos enviados por la KL25Z mediante UART. Después, Python interpreta los valores de ángulo y distancia para graficarlos en tiempo real.

La visualización puede hacerse en coordenadas polares o cartesianas.

---

## Librerías utilizadas

- PySerial
- NumPy
- Matplotlib

---

## Requisitos de Python

Instalar las librerías necesarias:

```bash
pip install pyserial numpy matplotlib
```

---

## Ejecutar visualización

```bash
python radar_display.py
```

Al ejecutar el programa, se debe seleccionar el puerto serial donde aparece la KL25Z.

---

## Puerto serial

El puerto puede variar dependiendo de la computadora.

Ejemplos comunes:

```txt
Windows: COM3, COM4, COM5
Linux: /dev/ttyUSB0 o /dev/ttyACM0
Mac: /dev/tty.usbmodemXXXX
```

---

# Estructura recomendada del proyecto

```txt
Mini-Radar-KL25Z/
│
├── README.md
├── KL25Z/
│   └── main.c
├── Python/
│   └── radar_display.py
└── diagrams/
    ├── diagrama_flujo.drawio
    ├── diagrama_conexiones.drawio
    └── diagrama_kl25.png
```

---

# Resultado esperado

El sistema debe:

- Mover el motor paso a paso de 0° a 180°.
- Regresar el motor de 180° a 0°.
- Medir distancia con el HC-SR04.
- Calcular el ángulo actual.
- Enviar datos por UART.
- Recibir los datos en Python.
- Mostrar una gráfica tipo radar en tiempo real.

---

# Diagrama de flujo del sistema

```txt
Inicio
  |
  v
Inicializar UART
  |
  v
Inicializar GPIO
  |
  v
Inicializar motor
  |
  v
Inicializar sensor ultrasónico
  |
  v
Mover motor un paso
  |
  v
Actualizar contador de pasos
  |
  v
Calcular ángulo
  |
  v
¿Han pasado 4 pasos?
  |
  ├── No -> Mover motor otro paso
  |
  └── Sí
        |
        v
   Medir distancia
        |
        v
   Enviar ANGULO,DISTANCIA por UART
        |
        v
   Continuar barrido
```

---

# Notas importantes

- No alimentar el motor directamente desde la KL25Z.
- Usar fuente externa de 5V para motor y sensor.
- Conectar todos los GND en común.
- Usar divisor de voltaje para el ECHO del HC-SR04.
- Verificar que Python y la KL25Z usen el mismo baud rate: `57600`.
- Revisar que el puerto serial seleccionado en Python sea el correcto.
- Evitar cables muy largos en el sensor ultrasónico.
- Verificar que el driver del motor esté correctamente alimentado.
- Confirmar que PTA2 esté configurado como UART0_TX.
- El motor puede requerir más corriente de la que entrega la KL25Z.

---

# Posibles problemas y soluciones

## No aparecen datos en Python

Revisar:

```txt
1. Que el puerto serial sea correcto.
2. Que el baud rate sea 57600.
3. Que PTA2 esté conectado correctamente.
4. Que la KL25Z esté enviando datos.
5. Que no haya otro programa usando el mismo puerto serial.
```

---

## El motor no se mueve

Revisar:

```txt
1. Que el motor tenga fuente externa.
2. Que el driver esté bien conectado.
3. Que GND de la fuente y GND de la KL25Z estén unidos.
4. Que las entradas IN1, IN2, IN3 e IN4 estén conectadas a PTD0-PTD3.
5. Que la secuencia de pasos sea correcta.
```

---

## El sensor no mide correctamente

Revisar:

```txt
1. Que TRIG esté conectado a PTB0.
2. Que ECHO esté conectado a PTB1 con divisor de voltaje.
3. Que VCC y GND estén correctamente conectados.
4. Que el objeto no esté demasiado cerca o demasiado lejos.
5. Que el tiempo de espera del ECHO tenga timeout.
```

---

## La gráfica se ve lenta

Puede deberse a:

```txt
1. Demasiadas mediciones por segundo.
2. Retardos muy largos en el motor.
3. Lecturas ultrasónicas demasiado frecuentes.
4. Puerto serial saturado.
```

Una solución es medir distancia cada cierto número de pasos, por ejemplo, cada 4 pasos.

---

# Conclusión

Este proyecto demuestra cómo un sistema embebido puede integrar movimiento, medición y comunicación en tiempo real.

La KL25Z actúa como controlador principal, coordinando el motor paso a paso, el sensor ultrasónico y la transmisión UART. Gracias a Python, los datos enviados por la tarjeta pueden visualizarse como un radar 2D.

El sistema permite aplicar conceptos importantes como control de GPIO, temporización, comunicación serial, adquisición de datos y visualización gráfica.

---
