# Practice 4 – Running/Paused State Control with Interrupts

## Overview

This project implements an interrupt-driven state control system on the FRDM-KL25Z microcontroller. The system uses a push button interrupt to switch the program into a paused state, a keypad input to return to the running state, an LCD to display the current status, and the TPM0 timer module to generate periodic timing events.

The main purpose of this practice is to demonstrate how external interrupts and timer interrupts can be combined to control the execution state of an embedded system.

## Features

- External interrupt using push button on PTA1
- TPM0 timer interrupt for periodic timing
- RUNNING and PAUSED system states
- LCD status display
- Keypad input to resume execution
- Register-level GPIO configuration
- Interrupt-based embedded control

## Hardware Used

- FRDM-KL25Z development board
- Push button
- 4x4 matrix keypad
- 16x2 LCD display
- Breadboard and jumper wires

## Pin Configuration

### Push Button

| Signal | KL25Z Pin |
|---|---|
| Push button interrupt | PTA1 |

The push button is configured with an internal pull-up resistor and generates an interrupt on the falling edge.

### Keypad

| Signal | KL25Z Pin |
|---|---|
| Row 1 | PTC0 |
| Row 2 | PTC1 |
| Row 3 | PTC2 |
| Row 4 | PTC3 |
| Column 1 | PTC4 |
| Column 2 | PTC5 |
| Column 3 | PTC6 |
| Column 4 | PTC7 |

### LCD 16x2

| Signal | KL25Z Pin |
|---|---|
| D0-D7 | PTD0-PTD7 |
| RS | PTA2 |
| RW | PTA4 |
| EN | PTA5 |

## How It Works

The system starts by disabling global interrupts while the peripherals are configured.

    __disable_irq();

Then the timer, push button, keypad, and LCD are initialized.

    Init_TPM0();
    Init_Button();
    keypad_init();
    LCD_init();

After initialization, the LCD displays the initial state.

    LCD_command(0x01);
    LCD_sendstring("RUNNING");

Finally, global interrupts are enabled.

    __enable_irq();

The program then enters the main loop, where it continuously reads the keypad and checks if the resume key has been pressed.

## System States

The program uses an enum to define two possible states:

    typedef enum { RUNNING, PAUSED } estado_t;

The current state is stored in a volatile variable:

    volatile estado_t estado = RUNNING;

The use of `volatile` is important because the state can be modified inside interrupt service routines.

## Push Button Interrupt

The push button is connected to PTA1 and configured to generate an interrupt on a falling edge.

    PORTA->PCR[1] = PORT_PCR_MUX(1) |
                    PORT_PCR_PE_MASK |
                    PORT_PCR_PS_MASK |
                    PORT_PCR_IRQC(0xA);

When the push button is pressed, the `PORTA_IRQHandler()` function is executed.

Inside the interrupt handler, the system changes to the paused state and updates the LCD.

    estado = PAUSED;

    LCD_command(0x01);
    LCD_sendstring("PAUSED");

The interrupt flag is cleared before leaving the handler.

    PORTA->ISFR = (1 << 1);

## Timer Interrupt

TPM0 is configured to generate periodic interrupts.

    TPM0->MOD = 37500;
    TPM0->SC = 0x07 | 0x40 | 0x08;

The timer interrupt handler clears the overflow flag and updates a counter only when the system is running.

    void TPM0_IRQHandler(void)
    {
        TPM0->SC |= 0x80;

        if (estado == PAUSED) return;

        contador_100ms++;
    }

If the system is paused, the interrupt returns without incrementing the counter.

This allows the timer to remain active while the system logic responds differently depending on the current state.

## Resume Using Keypad

The keypad is scanned continuously inside the main loop.

    char key = keypad_getkey();

If the `*` key is pressed, the system returns to the running state.

    if (key == 13)
    {
        delayMs(200);

        estado = RUNNING;
        contador_100ms = 0;

        LCD_command(0x01);
        LCD_sendstring("RUNNING");
    }

The counter is reset when the system resumes.

## Keypad Scanning

The keypad is connected to Port C. Rows are activated one at a time, and columns are read to detect which key was pressed.

If no key is pressed, the function returns:

    0

The returned key value is calculated based on the row and column position.

## LCD Operation

The LCD is configured in 8-bit mode using Port D as the data bus and Port A for control signals.

Commands are sent with:

    LCD_command();

Characters are sent with:

    LCD_data();

Text strings are printed using:

    LCD_sendstring();

The LCD is used to show the current state of the system:

    RUNNING

or

    PAUSED

## Code Structure

| Function | Description |
|---|---|
| `Init_TPM0()` | Configures TPM0 timer and enables its interrupt |
| `Init_Button()` | Configures PTA1 as an external interrupt input |
| `PORTA_IRQHandler()` | Handles the push button interrupt and pauses the system |
| `TPM0_IRQHandler()` | Handles TPM0 overflow interrupts |
| `keypad_init()` | Configures the keypad GPIO pins |
| `keypad_getkey()` | Scans the keypad and returns the pressed key |
| `LCD_init()` | Initializes the LCD in 8-bit mode |
| `LCD_command()` | Sends commands to the LCD |
| `LCD_data()` | Sends characters to the LCD |
| `LCD_sendstring()` | Prints text on the LCD |
| `delayMs()` | Generates millisecond delays |
| `delayUs()` | Generates microsecond delays |

## Expected Behavior

1. The system starts in RUNNING mode.
2. The LCD displays `RUNNING`.
3. TPM0 interrupts increment the internal counter while the system is running.
4. Pressing the push button on PTA1 triggers an external interrupt.
5. The system changes to PAUSED mode.
6. The LCD displays `PAUSED`.
7. While paused, the timer interrupt no longer increments the counter.
8. Pressing `*` on the keypad resumes the system.
9. The LCD displays `RUNNING` again.
10. The counter resets and the system continues operating.

## Skills Demonstrated

- Embedded C programming
- External interrupt configuration
- Timer interrupt handling
- State-based system design
- Matrix keypad scanning
- LCD interfacing
- Register-level programming
- Use of `volatile` variables in interrupt-driven systems
- GPIO configuration on the FRDM-KL25Z

## Conclusion

This practice demonstrates how to build an interrupt-driven embedded system using the FRDM-KL25Z. The project combines a push button interrupt, TPM0 timer interrupt, keypad input, and LCD output to control and display the system state. It shows how RUNNING and PAUSED modes can be managed using interrupts and shared state variables in an embedded application.
