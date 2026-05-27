#include "MKL25Z4.h"
#include <stdio.h>

#define RS 0x04
#define RW 0x10
#define EN 0x20

void ADC0_init(void);
void TPM0_init(void);
void delay_TPM0(int overflows);
void delayMs(int n);

void LCD_command(unsigned char command);
void LCD_data(unsigned char data);
void LCD_init(void);
void LCD_sendstring(char *str);

int main(void) {

    int result;
    float temperature;
    char buffer[16];

    ADC0_init();
    LCD_init();
    TPM0_init();


    SIM->SCGC5 |= 0x0400;        // clock Port B
    PORTB->PCR[18] = 0x100;      // GPIO
    PTB->PDDR |= (1 << 18);      // salida

    while (1) {


        ADC0->SC1[0] = 0;
        while (!(ADC0->SC1[0] & 0x80)) {}
        result = ADC0->R[0];


        temperature = result * 330.0 / 65536;


        sprintf(buffer, "%dF", (int)temperature);

        LCD_command(0x01);
        LCD_sendstring("Temperatura:");
        LCD_command(0xC0);
        LCD_sendstring(buffer);


        if (temperature > 320) {
            PTB->PCOR = (1 << 18);   // prender
        }
        else if (temperature < 120) {
            PTB->PSOR = (1 << 18);   // apagar
            delay_TPM0(20);          // esperar 2s
        }
        else {
            PTB->PSOR = (1 << 18);
        }
    }
}
void ADC0_init(void) {
    SIM->SCGC5 |= 0x2000;
    PORTE->PCR[20] = 0;

    SIM->SCGC6 |= 0x8000000;

    ADC0->SC2 &= ~0x40;
    ADC0->SC3 |= 0x07;

    ADC0->CFG1 = 0x40 | 0x10 | 0x0C;
}
void TPM0_init(void) {
    SIM->SCGC6 |= 0x01000000;
    SIM->SOPT2 |= 0x01000000;

    TPM0->SC = 0;
    TPM0->MOD = 37500;

    TPM0->SC = 0x07;            // Prescaler 128
    TPM0->SC |= 0x80;
    TPM0->SC |= 0x08;
}
void delay_TPM0(int overflows) {
    int i;
    for (i = 0; i < overflows; i++) {
        while ((TPM0->SC & 0x80) == 0) {}
        TPM0->SC |= 0x80;
    }
}
void LCD_init(void) {
    SIM->SCGC5 |= 0x1000;
    PORTD->PCR[0] = 0x100;
    PORTD->PCR[1] = 0x100;
    PORTD->PCR[2] = 0x100;
    PORTD->PCR[3] = 0x100;
    PORTD->PCR[4] = 0x100;
    PORTD->PCR[5] = 0x100;
    PORTD->PCR[6] = 0x100;
    PORTD->PCR[7] = 0x100;
    PTD->PDDR = 0xFF;

    SIM->SCGC5 |= 0x0200;
    PORTA->PCR[2] = 0x100;
    PORTA->PCR[4] = 0x100;
    PORTA->PCR[5] = 0x100;
    PTA->PDDR |= 0x34;

    delayMs(30);
    LCD_command(0x30);
    delayMs(10);
    LCD_command(0x30);
    delayMs(1);
    LCD_command(0x30);

    LCD_command(0x38);
    LCD_command(0x06);
    LCD_command(0x01);
    LCD_command(0x0F);
}

void LCD_command(unsigned char command) {
    PTA->PCOR = RS | RW;
    PTD->PDOR = command;
    PTA->PSOR = EN;
    delayMs(0);
    PTA->PCOR = EN;

    if (command < 4) delayMs(4);
    else delayMs(1);
}

void LCD_data(unsigned char data) {
    PTA->PSOR = RS;
    PTA->PCOR = RW;
    PTD->PDOR = data;
    PTA->PSOR = EN;
    delayMs(0);
    PTA->PCOR = EN;
    delayMs(1);
}

void LCD_sendstring(char *str) {
    while (*str) {
        LCD_data(*str++);
    }
}
void delayMs(int n) {
    volatile int i, j;
    for (i = 0; i < n; i++) {
        for (j = 0; j < 3000; j++) {
            __asm("nop");
        }
    }
}
