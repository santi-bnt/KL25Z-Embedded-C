/* * SoC Practice 1: RGB LED Control with User Interface
 * 4-BIT LCD MODE VERSION
 * This program scans a 4x4 matrix keypad and displays options on an LCD.
 * The RGB LED color changes based on the selection, confirming with '*'.
 */

#include <MKL25Z4.h>
#include <stdio.h>

// --- Function Declarations ---
void delayMs(int n);
void delayUs(int n);

void keypad_init(void);
unsigned char keypad_getkey(void);

void LED_init(void);
void LED_set(int value);

void LCD_init(void);
void LCD_nibble(unsigned char data); 
void LCD_command(unsigned char command);
void LCD_data(unsigned char data);
void LCD_sendstring(char *str);

// --- LCD Control Pins Definition ---
#define RS 0x04 /* PTA2 mask */
#define RW 0x10 /* PTA4 mask */
#define EN 0x20 /* PTA5 mask */

// ==============
// MAIN FUNCTION
// ==============
int main(void)
{
    unsigned char key;
    unsigned char last_key = 255; 
    unsigned char pending_color = 255; // Stores the choice before pressing '*'

    /* Peripheral Initialization */
    keypad_init();
    LED_init();
    LCD_init(); // Initializes in 4-bit mode

    /* Initial Screen Setup */
    LCD_command(0x01); // Clear screen
    delayMs(2);
    LCD_command(0x80); // Move cursor to line 1
    LCD_sendstring("Elige color:");
    LCD_command(0xC0); // Move cursor to line 2
    LCD_sendstring("Esperando...");

    /* Infinite Loop */
    for(;;)
    {
        key = keypad_getkey();

        // If no key is pressed, reset the last_key state so we can press the same key twice
        if (key == 255) 
        {
            last_key = 255;
        }
        // If a valid key is pressed and it's a new press (not held down)
        else if (key != last_key) 
        {
            last_key = key; 
            
            // 1. User presses a number to PRE-SELECT a color (0 to 7)
            if (key >= 0 && key <= 7)
            {
                pending_color = key; // Save it to memory, don't change the LED yet
                
                LCD_command(0xC0); 
                LCD_sendstring("                "); // Clear second line
                LCD_command(0xC0); 

                // Show the pending selection
                switch(pending_color)
                {
                    case 1: LCD_sendstring("Sel: 1-Rojo"); break;
                    case 2: LCD_sendstring("Sel: 2-Verde"); break;
                    case 3: LCD_sendstring("Sel: 3-Amar"); break;
                    case 4: LCD_sendstring("Sel: 4-Azul"); break;
                    case 5: LCD_sendstring("Sel: 5-Morado"); break;
                    case 6: LCD_sendstring("Sel: 6-Cian"); break;
                    case 7: LCD_sendstring("Sel: 7-Blanco"); break;
                    case 0: LCD_sendstring("Sel: 0-Apagado"); break;
                }
            }
            // 2. User presses the '*' key to CONFIRM (Mapped as 14 in the keypad matrix)
            else if (key == 14)
            {
                // Only act if the user actually pre-selected a valid color first
                if (pending_color != 255) 
                {
                    LED_set(pending_color); // Actually change the LED hardware
                    
                    LCD_command(0xC0); 
                    LCD_sendstring("                "); // Clear second line
                    LCD_command(0xC0); 

                    // Show the active confirmation
                    switch(pending_color)
                    {
                        case 1: LCD_sendstring("Activo: Rojo"); break;
                        case 2: LCD_sendstring("Activo: Verde"); break;
                        case 3: LCD_sendstring("Activo: Amar"); break;
                        case 4: LCD_sendstring("Activo: Azul"); break;
                        case 5: LCD_sendstring("Activo: Morado"); break;
                        case 6: LCD_sendstring("Activo: Cian"); break;
                        case 7: LCD_sendstring("Activo: Blanco"); break;
                        case 0: LCD_sendstring("Activo: Apagado"); break;
                    }
                }
            }
        }
    }
}

// =================
// KEYPAD FUNCTIONS
// =================
void keypad_init(void)
{
    SIM->SCGC5 |= 0x0800;  
    PORTC->PCR[0] = 0x103; 
    PORTC->PCR[1] = 0x103; 
    PORTC->PCR[2] = 0x103; 
    PORTC->PCR[3] = 0x103; 
    PORTC->PCR[4] = 0x103; 
    PORTC->PCR[5] = 0x103; 
    PORTC->PCR[6] = 0x103; 
    PORTC->PCR[7] = 0x103; 
    PTC->PDDR = 0x0F; 
}

unsigned char keypad_getkey(void) {
    int row, col;
    const char row_select[] = {0x01, 0x02, 0x04, 0x08};
    
    // Explicitly mapping physical keypad layout to integers
    // 14 represents the '*' key on the bottom left.
    const unsigned char keypad_map[4][4] = {
        { 1,  2,  3, 10}, 
        { 4,  5,  6, 11}, 
        { 7,  8,  9, 12}, 
        {14,  0, 15, 13}  
    };

    PTC->PDDR |= 0x0F; 
    PTC->PCOR = 0x0F;
    delayUs(2); 
    col = PTC-> PDIR & 0xF0; 
    PTC->PDDR = 0; 
    if (col == 0xF0) return 255; 

    for (row = 0; row < 4; row++)
    { 
        PTC->PDDR = 0; 
        PTC->PDDR |= row_select[row]; 
        PTC->PCOR = row_select[row]; 

        delayUs(2); 
        col = PTC->PDIR & 0xF0; 

        if (col != 0xF0) break;
    }

    PTC->PDDR = 0; 

    if (row == 4) return 255; 

    if (col == 0xE0) return keypad_map[row][0]; 
    if (col == 0xD0) return keypad_map[row][1]; 
    if (col == 0xB0) return keypad_map[row][2]; 
    if (col == 0x70) return keypad_map[row][3]; 

    return 255; 
}

// ==================
// RGB LED FUNCTIONS
// ==================
void LED_init(void)
{
    SIM->SCGC5 |= 0x400;  // Port B
    SIM->SCGC5 |= 0x1000; // Port D
    
    PORTB->PCR[18] = 0x100; 
    PTB->PDDR |= 0x40000; 
    PTB->PSOR |= 0x40000; 
    
    PORTB->PCR[19] = 0x100; 
    PTB->PDDR |= 0x80000; 
    PTB->PSOR |= 0x80000; 
    
    PORTD->PCR[1] = 0x100; 
    PTD->PDDR |= 0x02; 
    PTD->PSOR |= 0x02; 
}

void LED_set(int value)
{
    PTB->PSOR = 0x40000; 
    PTB->PSOR = 0x80000; 
    PTD->PSOR = 0x02;    

    switch(value)
    {
        case 1: PTB->PCOR = 0x40000; break; // Red
        case 2: PTB->PCOR = 0x80000; break; // Green
        case 3: PTB->PCOR = 0x40000; PTB->PCOR = 0x80000; break; // Yellow
        case 4: PTD->PCOR = 0x02; break; // Blue
        case 5: PTB->PCOR = 0x40000; PTD->PCOR = 0x02; break; // Purple
        case 6: PTB->PCOR = 0x80000; PTD->PCOR = 0x02; break; // Cyan
        case 7: PTB->PCOR = 0x40000; PTB->PCOR = 0x80000; PTD->PCOR = 0x02; break; // White
        default: break; 
    }
}

// ===============
// LCD FUNCTIONS (4-BIT MODE)
// ===============
void LCD_init(void)
{
    SIM->SCGC5 |= 0x1000; // Port D for Data (PTD4-PTD7)
    PORTD->PCR[4] = 0x100; 
    PORTD->PCR[5] = 0x100; 
    PORTD->PCR[6] = 0x100; 
    PORTD->PCR[7] = 0x100; 
    PTD->PDDR |= 0xF0; // PTD4, PTD5, PTD6, PTD7 as outputs
    
    SIM->SCGC5 |= 0x0200; // Port A for Control (PTA2, PTA4, PTA5)
    PORTA->PCR[2] = 0x100; 
    PORTA->PCR[4] = 0x100; 
    PORTA->PCR[5] = 0x100; 
    PTA->PDDR |= 0x34; 

    // 4-Bit Initialization Sequence
    delayMs(30); 
    PTA->PCOR = RS | RW; 
    
    LCD_nibble(0x30);
    delayMs(5);
    LCD_nibble(0x30);
    delayUs(150);
    LCD_nibble(0x30);
    delayUs(150);
    
    LCD_nibble(0x20); // Switch to 4-bit mode
    delayUs(150);

    // Now in 4-bit mode, send standard config commands
    LCD_command(0x28); // 4-bit, 2 lines, 5x7 font
    LCD_command(0x0C); // Display ON, Cursor OFF
    LCD_command(0x06); // Increment cursor, no shift
    LCD_command(0x01); // Clear screen
    delayMs(2);
}

void LCD_nibble(unsigned char data)
{
    // Clear the top 4 bits of Port D without affecting the rest (like PTD1)
    PTD->PCOR = 0xF0; 
    // Set the top 4 bits of Port D according to the data
    PTD->PSOR = (data & 0xF0);
    
    // Pulse EN
    PTA->PSOR = EN; 
    delayUs(50);
    PTA->PCOR = EN;
    delayUs(50);
}

void LCD_command(unsigned char command)
{
    PTA->PCOR = RS | RW; // RS = 0, RW = 0
    
    LCD_nibble(command & 0xF0);        // Send upper nibble
    LCD_nibble((command << 4) & 0xF0); // Send lower nibble
    
    if (command < 4) delayMs(2); 
    else delayUs(50); 
}

void LCD_data(unsigned char data)
{
    PTA->PSOR = RS; // RS = 1
    PTA->PCOR = RW; // RW = 0
    
    LCD_nibble(data & 0xF0);        // Send upper nibble
    LCD_nibble((data << 4) & 0xF0); // Send lower nibble
    
    delayUs(50);
}

void LCD_sendstring(char *str)
{
    while (*str != '\0')
    {
        LCD_data(*str);
        str++;
    }
}

// ================
// DELAY FUNCTIONS
// ================
void delayUs(int n) {
    volatile int i, j;
    for(i = 0; i < n; i++) {
        for(j = 0; j < 3; j++) {
            __asm("nop");
        }
    }
}

void delayMs(int n) {
    volatile int i, j;
    for(i = 0; i < n; i++) {
        for(j = 0; j < 3000; j++) {
            __asm("nop");
        }
    }
}