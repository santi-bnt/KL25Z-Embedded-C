/*
 * Part 1: Menu and Output Management
 * Displays a menu on LCD and turns on LED depending on keypad input
 */

#include <MKL25Z4.h>
#include <stdio.h>


void delayMs(int n);
void delayUs(int n);

void keypad_init(void);
unsigned char keypad_getkey(void);

void LED_init(void);
void LED_set(int value);

void LCD_init(void);
void LCD_byte(unsigned char data);
void LCD_command(unsigned char command);
void LCD_data(unsigned char data);
void LCD_sendstring(char *str);


#define RS 0x04
#define RW 0x10
#define EN 0x20


int main(void)
{
    unsigned char key;
    unsigned char last_key = 255;

    keypad_init();
    LED_init();
    LCD_init();

    for(;;)
    {

        LCD_command(0x01);
        delayMs(2);
        LCD_command(0x80);
        LCD_sendstring("PRESS BUTTON");
        LCD_command(0xC0);
        LCD_sendstring("R:1 B:2 G:3");

        LED_set(0); // apagar LED


        while (1)
        {
            key = keypad_getkey();

            if (key == 255)
                last_key = 255;
            else if (key != last_key)
            {
                last_key = key;

                // ROJO
                if (key == 1)
                {
                    LCD_command(0x01);
                    LCD_command(0x80);
                    LCD_sendstring("RED");
                    LCD_command(0xC0);
                    LCD_sendstring("LED IS ON!");

                    LED_set(1);
                    break;
                }
                //AZUL
                else if (key == 2)
                {
                    LCD_command(0x01);
                    LCD_command(0x80);
                    LCD_sendstring("BLUE");
                    LCD_command(0xC0);
                    LCD_sendstring("LED IS ON!");

                    LED_set(4);
                    break;
                }
                //VERDE
                else if (key == 3)
                {
                    LCD_command(0x01);
                    LCD_command(0x80);
                    LCD_sendstring("GREEN");
                    LCD_command(0xC0);
                    LCD_sendstring("LED IS ON!");

                    LED_set(2);
                    break;
                }
            }
        }

        delayMs(3000);
        LED_set(0);    // apagar LED
    }
}


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

unsigned char keypad_getkey(void)
{
    int row, col;
    const char row_select[] = {0x01, 0x02, 0x04, 0x08};

    const unsigned char keypad_map[4][4] = {
        {1,2,3,10},
        {4,5,6,11},
        {7,8,9,12},
        {14,0,15,13}
    };

    PTC->PDDR |= 0x0F;
    PTC->PCOR = 0x0F;
    delayUs(2);
    col = PTC->PDIR & 0xF0;
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


void LED_init(void)
{
    SIM->SCGC5 |= 0x400;
    SIM->SCGC5 |= 0x1000;

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
        case 1: PTB->PCOR = 0x40000; break; // rojo
        case 2: PTB->PCOR = 0x80000; break; // verde
        case 4: PTD->PCOR = 0x02; break;    // azul
    }
}


void LCD_init(void)
{
    SIM->SCGC5 |= 0x1000;
    PORTD->PCR[0] = 0x100;
    PORTD->PCR[1] = 0x100;
    PORTD->PCR[2] = 0x100;
    PORTD->PCR[3] = 0x100;
    PORTD->PCR[4] = 0x100;
    PORTD->PCR[5] = 0x100;
    PORTD->PCR[6] = 0x100;
    PORTD->PCR[7] = 0x100;
    PTD->PDDR |= 0xFF;

    SIM->SCGC5 |= 0x0200;
    PORTA->PCR[2] = 0x100;
    PORTA->PCR[4] = 0x100;
    PORTA->PCR[5] = 0x100;
    PTA->PDDR |= 0x34;

    delayMs(30);
    PTA->PCOR = RS | RW;

    LCD_command(0x30);
    delayMs(5);
    LCD_command(0x30);
    delayUs(150);
    LCD_command(0x30);
    delayUs(150);

    LCD_command(0x38);
    LCD_command(0x0C);
    LCD_command(0x06);
    LCD_command(0x01);
    delayMs(2);
}

void LCD_byte(unsigned char data)
{
    PTD->PCOR = 0xFF;
    PTD->PSOR = data;

    PTA->PSOR = EN;
    delayUs(50);
    PTA->PCOR = EN;
    delayUs(50);
}

void LCD_command(unsigned char command)
{
    PTA->PCOR = RS | RW;
    LCD_byte(command);

    if (command < 4) delayMs(2);
    else delayUs(50);
}

void LCD_data(unsigned char data)
{
    PTA->PSOR = RS;
    PTA->PCOR = RW;
    LCD_byte(data);
    delayUs(50);
}

void LCD_sendstring(char *str)
{
    while (*str)
        LCD_data(*str++);
}


void delayUs(int n)
{
    volatile int i,j;
    for(i=0;i<n;i++)
        for(j=0;j<3;j++)
            __asm("nop");
}

void delayMs(int n)
{
    volatile int i,j;
    for(i=0;i<n;i++)
        for(j=0;j<5000;j++)
            __asm("nop");
}
