# Mini Radar System with KL25Z

## Integrantes

Joshua Menchaca - Santiago Benavent - Jared García - André Pinto

---

## Descripción

Este proyecto implementa un sistema tipo radar usando una **KL25Z**, un **sensor ultrasónico HC-SR04**, un **motor paso a paso** y comunicación **UART** hacia una computadora.

El motor realiza un barrido de **0° a 180°**, el sensor mide la distancia y la KL25Z envía los datos a Python en el formato:

```txt
ANGULO,DISTANCIA
```

Ejemplo:

```txt
90,45
```

Python recibe los datos con **PySerial** y los grafica en tiempo real con **Matplotlib**.

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

---

## Funcionamiento general

```txt
1. La KL25Z mueve el motor paso a paso.
2. El motor realiza un barrido de 0° a 180°.
3. Cada 4 pasos, la KL25Z mide distancia con el HC-SR04.
4. La KL25Z calcula el ángulo actual.
5. La KL25Z envía ANGULO,DISTANCIA por UART.
6. Python recibe los datos y genera el radar 2D.
```

---

## Conexiones

### Sensor ultrasónico HC-SR04

| HC-SR04 | KL25Z | Función |
|---|---|---|
| VCC | 5V | Alimentación |
| GND | GND | Tierra común |
| TRIG | PTB0 | Disparo del sensor |
| ECHO | PTB1 | Lectura del eco |

> Importante: el pin ECHO puede entregar 5V. Se recomienda usar un divisor de voltaje para bajarlo a 3.3V antes de conectarlo a PTB1.

---

### Motor paso a paso

| Driver | KL25Z | Función |
|---|---|---|
| IN1 | PTD0 | Bobina 1 |
| IN2 | PTD1 | Bobina 2 |
| IN3 | PTD2 | Bobina 3 |
| IN4 | PTD3 | Bobina 4 |

---

### UART

| Señal | KL25Z | Función |
|---|---|---|
| TX | PTA2 | Envío de datos a la computadora |
| GND | GND | Tierra común |

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

## Comunicación UART

```txt
Baud rate: 57600
Formato enviado: ANGULO,DISTANCIA
Ejemplo: 90,45
```

La KL25Z solo transmite datos hacia la computadora usando UART0_TX en PTA2.

---

## Fórmulas importantes

### Distancia

```txt
distancia = (tiempo * 0.0343) / 2
```

### Ángulo

```txt
angulo = (pasos_totales * 180) / 1024
```

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

Al ejecutar el programa, seleccionar el puerto serial donde aparece la KL25Z.

---

## Estructura recomendada

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
    └── diagrama_conexiones.drawio
```

---

## Resultado esperado

El sistema debe:

- Mover el motor de 0° a 180°.
- Medir distancia con el HC-SR04.
- Enviar datos por UART.
- Recibir los datos en Python.
- Mostrar una gráfica tipo radar en tiempo real.

---

## Notas importantes

- No alimentar el motor directamente desde la KL25Z.
- Usar fuente externa de 5V para motor y sensor.
- Conectar todos los GND en común.
- Usar divisor de voltaje para el ECHO del HC-SR04.
- Verificar que Python y la KL25Z usen el mismo baud rate: `57600`.
