/* A to D conversion of channel 0
 * This program converts the analog input from channel 0 (PTE20)
 * using software trigger continuously.
 * Bits 10-8 are used to control the tri-color LEDs. LED code * is  copied from p2_7. Connect a potentiometer between 3.3V * and  ground.
 * The wiper of the potentiometer is connected to PTE20.
 When the potentiometer is turned, the LEDs should change   color. */

#include "MKL25Z4.h"
void ADC0_init(void);
void LED_set(int s);
void LED_init(void);
void LCD_init(void);
void LCD_data(unsigned char data);
void LCD_command(unsigned char command);
void LCD_sendstring(char * str);
void delayMs(int n);
#define RS 0x04
#define RW 0x10
#define EN 0x20
int main(void) {
	short int result;
	LED_init(); /* Configure LEDs */
	ADC0_init(); /* Configure ADC0 */
	LCD_init();
	int voltaje=0;
	int unit=0;
	int dec=0;

    LCD_command(0x01);
    LCD_sendstring("Voltaje obtenido: ");


	while (1) {
		ADC0->SC1[0] = 0; /* start conversion on channel 0 */


		while (!(ADC0->SC1[0] & 0x80)) {
		} /* wait COCO */
		result = ADC0->R[0];
		/* read conversion result and clear COCO flag */

		LED_set(result >> 7); /* display result on LED */

		voltaje= (result*3.3*1000)/4096;

		unit=voltaje/1000; //unidad
		dec=(voltaje%1000)/10; //decimal

		LCD_command(0xC0);
        LCD_data(unit+'0');
        LCD_data('.');
        LCD_data((dec/10)+'0');
	}
}
void ADC0_init(void) {

	SIM->SCGC5 |= 0x2000; /* clock to PORTE */

	PORTE->PCR[20] = 0; /* PTE20 analog input */

	SIM->SCGC6 |= 0x8000000; /* clock to ADC0 */

	ADC0->SC2 &= ~0x40; /* software trigger */

	/* clock div by 4, long sample time, single ended 12 bit, bus clock */

	ADC0->CFG1 = 0x40 | 0x10 | 0x04 | 0x00;
}
void LED_init(void) {

	SIM->SCGC5 |= 0x400; /* enable clock to Port B */

	SIM->SCGC5 |= 0x1000; /* enable clock to Port D */

	PORTB->PCR[18] = 0x100; /* make PTB18 pin as GPIO */

	PTB->PDDR |= 0x40000; /* make PTB18 as output pin */

	PORTB->PCR[19] = 0x100; /* make PTB19 pin as GPIO */

	PTB->PDDR |= 0x80000; /* make PTB19 as output pin */

	PORTD->PCR[1] = 0x100; /* make PTD1 pin as GPIO */

	PTD->PDDR |= 0x02;
} /* make PTD1 as output pin */
void LED_set(int s) {

	if (s & 1) /* use bit 0 of s to control red LED */
		PTB->PCOR = 0x40000; /* turn on red LED */
	else
		PTB->PSOR = 0x40000; /* turn off red LED */

	if (s & 2) /* use bit 1 of s to control green LED */
		PTB->PCOR = 0x80000; /* turn on green LED */
	else
		PTB->PSOR = 0x80000; /* turn off green LED */

	if (s & 4) /* use bit 2 of s to control blue LED */
		PTD->PCOR = 0x02; /* turn on blue LED */
	else
		PTD->PSOR = 0x02; /* turn off blue LED */
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
