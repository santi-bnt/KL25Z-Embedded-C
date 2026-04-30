# ADC Potentiometer – Voltage Monitor with LCD and RGB LED

## Overview

This project implements an analog voltage monitoring system using the FRDM-KL25Z microcontroller. The system reads a potentiometer through the ADC module, converts the digital result into a voltage value, displays the result on a 16x2 LCD, and uses the onboard RGB LED as a visual indicator of the input level.

The system runs continuously, so changes in the potentiometer position are reflected in real time.

## Features

- Analog input reading using ADC0
- Potentiometer-based voltage measurement
- LCD voltage visualization
- RGB LED level indication
- 12-bit ADC conversion
- Real-time embedded processing
- Register-level peripheral configuration

## Hardware Used

- FRDM-KL25Z development board
- Potentiometer
- 16x2 LCD display
- Onboard RGB LED
- Breadboard and jumper wires

## Pin Configuration

### Potentiometer   

| Component | KL25Z Pin |
|---|---|
| Potentiometer wiper | PTE20 |
| Side terminal | 3.3V |
| Side terminal | GND |

### LCD 16x2

| Signal | KL25Z Pin |
|---|---|
| D0-D7 | PTD0-PTD7 |
| RS | PTA2 |
| RW | PTA4 |
| EN | PTA5 |

### RGB LED

| Color | KL25Z Pin |
|---|---|
| Red | PTB18 |
| Green | PTB19 |
| Blue | PTD1 |

The onboard RGB LED uses active-low logic. Writing `0` turns the LED on, while writing `1` turns it off.

## How It Works

The system starts by initializing the RGB LED, ADC0, and LCD.

    LED_init();
    ADC0_init();
    LCD_init();

After initialization, the program enters an infinite loop where it continuously starts an ADC conversion, waits for the conversion to finish, reads the result, processes the value, updates the LED state, and displays the voltage on the LCD.

## ADC Operation

The ADC conversion is started by writing to the ADC channel register.

    ADC0->SC1[0] = 0;

The program waits until the conversion complete flag is active.

    while (!(ADC0->SC1[0] & 0x80)) {
    }

Then, the ADC result is read.

    result = ADC0->R[0];

The ADC is configured for 12-bit resolution, so the digital output ranges from `0` to `4095`.

## Voltage Calculation

The ADC value is converted into millivolts using a 3.3 V reference.

    voltaje = (result * 3.3 * 1000) / 4096;

The value is then separated into integer and decimal components.

    unit = voltaje / 1000;
    dec = (voltaje % 1000) / 10;

This allows the voltage to be displayed in decimal format on the LCD.

## RGB LED Indicator

The ADC result is shifted before being sent to the LED control function.

    LED_set(result >> 7);

The most significant bits of the ADC result are used to control the RGB LED.

| Bit | LED Color |
|---|---|
| Bit 0 | Red |
| Bit 1 | Green |
| Bit 2 | Blue |

As the potentiometer changes position, the ADC value changes and produces different RGB LED combinations.

## LCD Display

The LCD is configured in 8-bit mode using Port D as the data bus and Port A as the control bus.

The voltage value is written character by character on the display.

    LCD_data(unit + '0');
    LCD_data('.');
    LCD_data((dec / 10) + '0');

The expression `+ '0'` converts numeric values into printable ASCII characters.

## Code Structure

| Function | Description |
|---|---|
| `ADC0_init()` | Configures ADC0 for analog input reading |
| `LED_init()` | Configures RGB LED pins as outputs |
| `LED_set()` | Updates the RGB LED based on ADC bits |
| `LCD_init()` | Initializes the LCD in 8-bit mode |
| `LCD_command()` | Sends commands to the LCD |
| `LCD_data()` | Sends characters to the LCD |
| `LCD_sendstring()` | Prints strings on the LCD |
| `delayMs()` | Provides timing delays for LCD operation |

## Expected Behavior

1. The potentiometer outputs an analog voltage.
2. ADC0 reads the signal through PTE20.
3. The ADC result is converted into a voltage value.
4. The LCD displays the measured voltage.
5. The RGB LED changes color based on the input level.
6. The system updates continuously in real time.

## Skills Demonstrated

- Embedded C programming
- ADC configuration
- Analog signal processing
- GPIO output control
- LCD interfacing
- Bit manipulation
- Register-level programming
- Real-time embedded system design

## Conclusion

This practice demonstrates how to acquire and process an analog signal using the FRDM-KL25Z ADC module. The project integrates ADC reading, LCD visualization, and RGB LED feedback to create a real-time voltage monitoring system.
