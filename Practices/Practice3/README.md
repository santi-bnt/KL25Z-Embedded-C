# Practice 3 – Keypad Timer and RGB LED Interface

## Overview

This project implements an embedded interface on the FRDM-KL25Z microcontroller using a 4x4 matrix keypad, a 16x2 LCD, the onboard RGB LED, and the TPM0 timer module.

The system allows the user to enter a time value through the keypad, confirm the input, and start a timer. While the timer is running, the LCD displays the elapsed time. When the selected time is reached, the system displays a completion message and turns on the red LED.

## Features

- 4x4 matrix keypad input
- LCD user interface
- Numeric input buffer
- Confirmation using the `*` key
- Timer-based counting using TPM0 interrupts
- RGB LED completion indicator
- Register-level GPIO configuration

## Hardware Used

- FRDM-KL25Z development board
- 4x4 matrix keypad
- 16x2 LCD display
- Onboard RGB LED
- Breadboard and jumper wires

## Pin Configuration

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

### RGB LED

| Color | KL25Z Pin |
|---|---|
| Red | PTB18 |
| Green | PTB19 |
| Blue | PTD1 |

The onboard RGB LED uses active-low logic. This means that writing `0` turns the LED on, while writing `1` turns it off.

## How It Works

The system starts by initializing the keypad, RGB LED, LCD, and TPM0 timer.

    keypad_init();
    LED_init();
    LCD_init();
    TPM0_init();

After initialization, the LCD displays a short welcome message and then asks the user to enter the number of seconds.

The keypad is scanned continuously inside the main loop. When the user presses a numeric key, the value is stored in a character buffer and displayed on the second line of the LCD.

    buffer[index++] = key + '0';
    buffer[index] = '\0';

The user can enter up to four digits. This allows the system to handle multi-digit time values.

## Timer Confirmation

The timer starts when the user presses the `*` key.

In the keypad map, the `*` key is represented by the value `14`.

    else if (key == 14)

When the confirmation key is pressed, the buffer is converted into an integer.

    segundos = atoi(buffer);

Then the timer variables are reset and the system enters counting mode.

    index = 0;
    buffer[0] = '\0';
    contador = 0;
    running = 1;

While the timer is running, the LCD displays the current progress using the following format:

    t=current/target

For example:

    t=4/10

This means that 4 seconds have passed out of the 10 seconds selected by the user.

## TPM0 Timer Interrupt

TPM0 is configured to generate periodic interrupts. The interrupt service routine updates a tick counter while the timer is running.

    void TPM0_IRQHandler(void)
    {
        TPM0->SC |= 0x80;

        static int ticks = 0;

        if (running)
        {
            ticks++;

            if (ticks >= 10)
            {
                contador++;
                ticks = 0;
            }
        }
    }

Each group of ticks represents approximately one second. This allows the system to count time using a hardware timer instead of depending only on blocking delay loops.

## Completion Behavior

When the elapsed time reaches the value selected by the user, the system stops the timer, clears the LCD, displays a final message, and turns on the red LED.

    if (contador >= segundos)
    {
        running = 0;

        LCD_command(0x01);
        LCD_sendstring("FIN!");

        LED_set(1);
    }

The red LED works as a visual indicator that the selected time has finished.

## Keypad Mapping

| Key | Returned Value |
|---|---|
| 1 | 1 |
| 2 | 2 |
| 3 | 3 |
| A | 10 |
| 4 | 4 |
| 5 | 5 |
| 6 | 6 |
| B | 11 |
| 7 | 7 |
| 8 | 8 |
| 9 | 9 |
| C | 12 |
| * | 14 |
| 0 | 0 |
| # | 15 |
| D | 13 |

If no key is pressed, the function returns `255`. This value is used to detect an idle keypad state.

## Code Structure

| Function | Description |
|---|---|
| `keypad_init()` | Configures the keypad GPIO pins |
| `keypad_getkey()` | Scans the keypad and returns the pressed key |
| `LED_init()` | Configures the RGB LED pins |
| `LED_set()` | Controls the RGB LED state |
| `LCD_init()` | Initializes the LCD display |
| `LCD_command()` | Sends commands to the LCD |
| `LCD_data()` | Sends characters to the LCD |
| `LCD_sendstring()` | Prints strings on the LCD |
| `TPM0_init()` | Configures the TPM0 timer |
| `TPM0_IRQHandler()` | Handles TPM0 overflow interrupts |

## Expected Behavior

1. The LCD displays a welcome message.
2. The system asks the user to enter a number of seconds.
3. The user enters the value using the keypad.
4. The user confirms the input by pressing `*`.
5. The LCD displays the current timer progress.
6. TPM0 interrupts update the elapsed time.
7. When the selected time is reached, the LCD displays `FIN!`.
8. The red LED turns on as a completion indicator.

## Skills Demonstrated

- Embedded C programming
- GPIO configuration
- Matrix keypad scanning
- LCD interfacing
- Timer interrupt handling
- Register-level programming
- User input processing
- Embedded user interface design

## Conclusion

This practice demonstrates how to combine keypad input, LCD feedback, RGB LED control, and hardware timer interrupts in a single embedded application. The project shows how the FRDM-KL25Z can be used to build an interactive timer system with real-time user input and visual output feedback.
