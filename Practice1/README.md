# Practice 1 – RGB LED Control with Keypad and LCD Interface

## Overview

This project implements an embedded user interface on the FRDM-KL25Z microcontroller. The system allows the user to select an RGB LED color using a 4x4 matrix keypad, preview the selected option on a 16x2 LCD, and confirm the selection using the `*` key.

The project combines keypad scanning, LCD communication in 4-bit mode, GPIO control, and active-low RGB LED logic.

## Features

- 4x4 matrix keypad input
- LCD-based user interface
- RGB LED color selection
- Confirmation system using the `*` key
- LCD communication in 4-bit mode
- Active-low LED control
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
| D4-D7 | PTD4-PTD7 |
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

The system starts by initializing the keypad, RGB LED, and LCD.

    keypad_init();
    LED_init();
    LCD_init();

The LCD is configured in 4-bit mode to reduce the number of GPIO pins used. This also avoids conflicts with the blue LED pin.

After initialization, the system displays a color selection interface. The keypad is scanned continuously inside the main loop.

When the user presses a number from `0` to `7`, the selected color is stored as a pending option. The LED does not change immediately. The selected option is shown on the LCD.

The color is only applied when the user presses the `*` key.

## Color Selection

| Key | LED State |
|---|---|
| 0 | Off |
| 1 | Red |
| 2 | Green |
| 3 | Yellow |
| 4 | Blue |
| 5 | Purple |
| 6 | Cyan |
| 7 | White |

## Confirmation Logic

The program uses the variable `pending_color` to store the selected color before confirmation.

    pending_color = key;

When the `*` key is pressed, the stored value is applied to the RGB LED.

    LED_set(pending_color);

This prevents accidental changes and creates a simple two-step user interaction: select first, confirm second.

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

If no key is pressed, the function returns `255`. This value is used to identify the idle state of the keypad.

## Code Structure

| Function | Description |
|---|---|
| `keypad_init()` | Configures the keypad GPIO pins |
| `keypad_getkey()` | Scans the keypad and returns the pressed key |
| `LED_init()` | Configures RGB LED pins as outputs |
| `LED_set()` | Applies the selected RGB color |
| `LCD_init()` | Initializes the LCD in 4-bit mode |
| `LCD_nibble()` | Sends 4-bit data to the LCD |
| `LCD_command()` | Sends commands to the LCD |
| `LCD_data()` | Sends characters to the LCD |
| `LCD_sendstring()` | Prints strings on the LCD |

## Expected Behavior

1. The LCD displays the color selection interface.
2. The user selects a color using keys `0` to `7`.
3. The LCD displays the selected option.
4. The user confirms using `*`.
5. The RGB LED changes to the confirmed color.
6. The system continues scanning for new selections.

## Skills Demonstrated

- Embedded C programming
- GPIO configuration
- Matrix keypad scanning
- LCD interfacing in 4-bit mode
- RGB LED control
- Active-low logic handling
- Register-level programming
- Embedded user interface design

## Conclusion

This practice demonstrates how to build a simple embedded user interface using keypad input, LCD feedback, and RGB LED output. The project shows how multiple peripherals can be integrated efficiently on the FRDM-KL25Z while keeping the interaction clear and controlled through a confirmation-based workflow.