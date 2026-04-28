# Temperature Sensor – LCD Monitor and Output Control

## Overview

This project implements a temperature monitoring system using the FRDM-KL25Z microcontroller. The system reads an analog signal through ADC0, converts the result into a temperature value, displays the measurement on a 16x2 LCD, and controls a digital output depending on defined temperature thresholds.

The system runs continuously and updates the display and output state in real time.

## Features

- Analog temperature reading using ADC0
- Temperature calculation from ADC data
- LCD-based temperature display
- Digital output control based on thresholds
- TPM0-based delay generation
- Register-level GPIO configuration
- Real-time monitoring loop

## Hardware Used

- FRDM-KL25Z development board
- Analog temperature sensor or temperature input circuit
- 16x2 LCD display
- Digital output connected to PTB18
- Breadboard and jumper wires

## Pin Configuration

### Analog Input

| Signal | KL25Z Pin |
|---|---|
| Analog temperature input | PTE20 |

### LCD 16x2

| Signal | KL25Z Pin |
|---|---|
| D0-D7 | PTD0-PTD7 |
| RS | PTA2 |
| RW | PTA4 |
| EN | PTA5 |

### Output

| Signal | KL25Z Pin |
|---|---|
| Temperature indicator output | PTB18 |

The output uses active-low logic. Clearing the pin turns the output on, while setting the pin turns it off.

## How It Works

The system starts by initializing ADC0, the LCD, and TPM0.

    ADC0_init();
    LCD_init();
    TPM0_init();

Then, PTB18 is configured as a digital output.

    SIM->SCGC5 |= 0x0400;
    PORTB->PCR[18] = 0x100;
    PTB->PDDR |= (1 << 18);

After initialization, the program enters an infinite loop where it continuously reads the analog input, calculates temperature, updates the LCD, and checks the threshold conditions.

## ADC Operation

The ADC conversion starts by selecting channel 0.

    ADC0->SC1[0] = 0;

The program waits until the conversion is complete.

    while (!(ADC0->SC1[0] & 0x80)) {}

Then, the result is read.

    result = ADC0->R[0];

The ADC result is used as the input for the temperature calculation.

## Temperature Calculation

The ADC result is scaled into a temperature value using the following expression:

    temperature = result * 330.0 / 65536;

The calculated temperature is converted into a text string before being displayed.

    sprintf(buffer, "%dF", (int)temperature);

This allows the LCD to show the temperature value in Fahrenheit format.

## LCD Display

The LCD is used to display the current temperature value.

    LCD_command(0x01);
    LCD_sendstring("Temperatura:");
    LCD_command(0xC0);
    LCD_sendstring(buffer);

The first line shows the measurement label, and the second line shows the calculated temperature.

## Output Control Logic

The system compares the temperature against two threshold values.

### High Temperature

If the temperature is greater than `320°F`, the output turns on.

    if (temperature > 320) {
        PTB->PCOR = (1 << 18);
    }

### Low Temperature

If the temperature is lower than `120°F`, the output turns off and the system waits using TPM0.

    else if (temperature < 120) {
        PTB->PSOR = (1 << 18);
        delay_TPM0(20);
    }

### Normal Range

If the temperature is between both thresholds, the output remains off.

    else {
        PTB->PSOR = (1 << 18);
    }

## TPM0 Delay

TPM0 is configured as a hardware timer to generate controlled delays.

    TPM0->MOD = 37500;
    TPM0->SC = 0x07;
    TPM0->SC |= 0x08;

The delay function waits for a selected number of TPM0 overflows.

    delay_TPM0(20);

This provides a timer-based delay instead of depending only on software loops.

## Code Structure

| Function | Description |
|---|---|
| `ADC0_init()` | Configures ADC0 for analog input reading |
| `TPM0_init()` | Configures TPM0 for delay generation |
| `delay_TPM0()` | Creates a delay using TPM0 overflow |
| `LCD_init()` | Initializes the LCD |
| `LCD_command()` | Sends commands to the LCD |
| `LCD_data()` | Sends characters to the LCD |
| `LCD_sendstring()` | Prints strings on the LCD |
| `delayMs()` | Provides LCD timing delays |

## Expected Behavior

1. The analog temperature signal is read through ADC0.
2. The ADC value is converted into a temperature value.
3. The LCD displays the current temperature.
4. If the temperature is above `320°F`, the output turns on.
5. If the temperature is below `120°F`, the output turns off and a timer delay is applied.
6. If the temperature is within the normal range, the output remains off.
7. The system continues updating in real time.

## Skills Demonstrated

- Embedded C programming
- ADC configuration
- Analog signal processing
- LCD interfacing
- GPIO output control
- Timer-based delay generation
- Register-level programming
- Conditional control logic

## Conclusion

This practice demonstrates how to build a temperature monitoring and response system using the FRDM-KL25Z. The project integrates ADC reading, LCD visualization, timer-based delays, and output control to show how an embedded system can process analog data and react to predefined conditions.
