/*
Part 1
//Program (1): Interrupt from a switch

//PORTA interrupt from a switch

//Upon pressing a switch connecting either PTA1 or PTA2 to ground, the green LED will toggle for three times.
//Main program toggles red LED while waiting for interrupt from switches.

#include <MKL25Z4.H>

void delayMs(int n);

int main(void) {

	// Init code for timer
	SIM->SCGC6 |= 0x01000000; // enable clock to TPM0
	SIM->SOPT2 |= 0x01000000; // use 32.76khz as clock
	TPM0->SC = 0; // disable timer while configuring
	TPM0->SC = 0x02; // prescaler /4
	TPM0->MOD = 0x2000; // max modulo value 8192
	TPM0->SC |= 0x80; // clear TOF
	TPM0->SC |= 0x08; // enable timer free-running mode



	__disable_irq(); // disable all IRQs

	SIM->SCGC5 |= 0x400; // enable clock to Port B
	PORTB->PCR[18] = 0x100; // make PTB18 pin as GPIO
	PORTB->PCR[19] = 0x100; // make PTB19 pin as GPIO
	PTB->PDDR |= 0xC0000; // make PTB18, 19 as output pin
	PTB->PDOR |= 0xC0000; // turn off LEDs
	SIM->SCGC5 |= 0x200; // enable clock to Port A

	// configure PTA1 for interrupt
	PORTA->PCR[1] |= 0x00100; // make it GPIO
	PORTA->PCR[1] |= 0x00003; // enable pull-up
	PTA->PDDR &= ~0x0002; // make pin input
	PORTA->PCR[1] &= ~0xF0000; // clear interrupt selection
	PORTA->PCR[1] |= 0xA0000; // enable falling edge INT

	while(1)
	{// toggle the red LED continuously
		PTB->PTOR |= 0x40000; // toggle red LED
		delayMs(500);
	}
}

// A pushbutton switch is connecting either PTA1 or PTA2 to ground to trigger PORTA interrupt
void PORTA_IRQHandler(void) {
	int i;

	for (i = 0; i < 3; i++) {
		PTB->PDOR &= ~0x80000; // LED verde ON
		delayMs(500);
		PTB->PDOR |= 0x80000;  // LED verde OFF
		delayMs(500);
	}

	PORTA->ISFR = 0x00000002; // limpiar solo PTA1
}

// Delay function
void delayMs(int n) {
	for(int i = 0; i < n; i++)
	{
		while((TPM0->SC & 0x80) == 0) { }
		// wait until the TOF is set
		TPM0->SC |= 0x80; // clear TOF
	}
}
 */

/*
Part 2
//Program (2): Multiple requests from single port

// Toggles red LED while waiting for INT from SWs
#include <MKL25Z4.h>

void delayMs(int n);

int main(void)
{
	// Init code for timer

	SIM->SCGC6 |= 0x01000000; // enable clock to TPM0
	SIM->SOPT2 |= 0x01000000; // use 32.76khz as clock
	TPM0->SC = 0; // disable timer while configuring
	TPM0->SC = 0x02; // prescaler /4
	TPM0->MOD = 0x2000; // max modulo value 8192
	TPM0->SC |= 0x80; // clear TOF
	TPM0->SC |= 0x08; // enable timer free-running mode

	__disable_irq(); // disable all IRQs
	SIM->SCGC5 |= 0x400; // enable clock to Port B
	SIM->SCGC5 |= 0x1000; // enable clock to Port D
	PORTB->PCR[18] = 0x100; // make PTB18 pin as GPIO
	PORTB->PCR[19] = 0x100; // make PTB19 pin as GPIO
	PTB->PDDR |= 0xC0000; // make PTB18-19 as output pin
	PORTD->PCR[1] = 0x100; // make PTD1 pin as GPIO
	PTD->PDDR |= 0x02; // make PTD1 as output pin
	PTB->PDOR |= 0xC0000; // turn off red/green LEDs
	PTD->PDOR |= 0x02; // turn off blue LED
	SIM->SCGC5 |= 0x200; // enable clock to Port A

	// configure PTA1 for interrupt
	PORTA->PCR[1] |= 0x00100; // make it GPIO
	PORTA->PCR[1] |= 0x00003; // enable pull-up
	PTA->PDDR &= ~0x0002; // make pin input
	PORTA->PCR[1] &= ~0xF0000; // clear interrupt selection
	PORTA->PCR[1] |= 0xA0000; // enable falling edge INT

	// configure PTA2 for interrupt
	PORTA->PCR[2] |= 0x00100; // make it GPIO
	PORTA->PCR[2] |= 0x00003; // enable pull-up
	PTA->PDDR &= ~0x0004; // make pin input
	PORTA->PCR[2] &= ~0xF0000; // clear interrupt selection
	PORTA->PCR[2] |= 0xA0000; // enable falling edge INT
	NVIC->ISER[0] |= 0x40000000; // enable INT30 (bit 30 of ISER[0])
	__enable_irq(); // global enable IRQs

	// toggle the red LED continuously
	while(1)
	{
		PTB->PTOR |= 0x40000; // toggle red LED
		delayMs(500);
	}
}

void PORTA_IRQHandler(void) {
	int i;
	while (PORTA->ISFR & 0x00000006) {
		if (PORTA->ISFR & 0x00000002) {
			// toggle green LED (PTB19) three times
			for (i = 0; i < 3; i++)
			{
				PTB->PDOR &= ~0x80000; // turn on green LED
				delayMs(500);
				PTB->PDOR |= 0x80000; // turn off green LED
				delayMs(500);
			}
			PORTA->ISFR = 0x00000002; // clear interrupt flag
			}

		if (PORTA->ISFR & 0x00000004) {
			// toggle blue LED (PTD1) three times
			for (i = 0; i < 3; i++)
			{
				PTD->PDOR &= ~0x02; // turn on blue LED
				delayMs(500);
				PTD->PDOR |= 0x02; // turn off blue LED
				delayMs(500);
			}
			PORTA->ISFR = 0x00000004; // clear interrupt flag
			}
	}
}

// Delay function
void delayMs(int n)
{
	for(int i = 0; i < n; i++)
	{
		while((TPM0->SC & 0x80) == 0){}
		// wait until the TOF is set
		TPM0->SC |= 0x80; // clear TOF
	}
}

 */

/*
Part 3
 */
#include "MKL25Z4.h"
#include <stdio.h>

#define RS 0x04
#define RW 0x10
#define EN 0x20

typedef enum { RUNNING, PAUSED } estado_t;
volatile estado_t estado = RUNNING;
volatile uint8_t contador_100ms = 0;

void Init_TPM0(void);
void Init_Button(void);

void keypad_init(void);
char keypad_getkey(void);

void LCD_init(void);
void LCD_command(unsigned char command);
void LCD_data(unsigned char data);
void LCD_sendstring(char *str);

void delayMs(int n);
void delayUs(int n);

//MAIN
int main(void)
{
    __disable_irq();

    Init_TPM0();
    Init_Button();
    keypad_init();
    LCD_init();

    LCD_command(0x01);
    LCD_sendstring("RUNNING");

    __enable_irq();

    while (1)
    {
        char key = keypad_getkey();

        if (key == 13) // '*'
        {
            delayMs(200); 

            estado = RUNNING;
            contador_100ms = 0;

            LCD_command(0x01);
            LCD_sendstring("RUNNING");
        }

        delayMs(10);
    }
}

//BUTTON
void Init_Button(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;

    PORTA->PCR[1] = PORT_PCR_MUX(1) |
                    PORT_PCR_PE_MASK |
                    PORT_PCR_PS_MASK |
                    PORT_PCR_IRQC(0xA);

    PTA->PDDR &= ~(1 << 1);

    NVIC_EnableIRQ(PORTA_IRQn);
}

void PORTA_IRQHandler(void)
{
    if (PORTA->ISFR & (1 << 1))
    {
        estado = PAUSED;

        LCD_command(0x01);
        LCD_sendstring("PAUSED");

        PORTA->ISFR = (1 << 1);
    }
}

//TIMER
void Init_TPM0(void)
{
    SIM->SCGC6 |= 0x01000000;
    SIM->SOPT2 |= 0x01000000;

    TPM0->SC = 0;
    TPM0->MOD = 37500;

    TPM0->SC = 0x07 | 0x40 | 0x08;

    NVIC_EnableIRQ(TPM0_IRQn);
}

void TPM0_IRQHandler(void)
{
    TPM0->SC |= 0x80;

    if (estado == PAUSED) return;

    contador_100ms++;

}

//KEYPAD
void keypad_init(void)
{
    SIM->SCGC5 |= 0x0800;

    for (int i = 0; i < 8; i++)
        PORTC->PCR[i] = 0x103;

    PTC->PDDR = 0x0F;
}

char keypad_getkey(void)
{
    int row, col;
    const char row_select[] = {0x01,0x02,0x04,0x08};

    PTC->PDDR |= 0x0F;
    PTC->PCOR = 0x0F;
    delayUs(2);

    col = PTC->PDIR & 0xF0;
    PTC->PDDR = 0;

    if (col == 0xF0) return 0;

    for (row = 0; row < 4; row++)
    {
        PTC->PDDR = 0;
        PTC->PDDR |= row_select[row];
        PTC->PCOR = row_select[row];

        delayUs(1);
        col = PTC->PDIR & 0xF0;

        if (col != 0xF0) break;
    }

    PTC->PDDR = 0;

    if (row == 4) return 0;

    if (col == 0xE0) return row*4+1;
    if (col == 0xD0) return row*4+2;
    if (col == 0xB0) return row*4+3;
    if (col == 0x70) return row*4+4;

    return 0;
}

//LCD
void LCD_init(void)
{
    SIM->SCGC5 |= 0x1000;
    for(int i=0;i<8;i++) PORTD->PCR[i]=0x100;
    PTD->PDDR = 0xFF;

    SIM->SCGC5 |= 0x0200;
    PORTA->PCR[2]=PORTA->PCR[4]=PORTA->PCR[5]=0x100;
    PTA->PDDR |= 0x34;

    delayMs(30);
    LCD_command(0x30); delayMs(10);
    LCD_command(0x30); delayMs(1);
    LCD_command(0x30);

    LCD_command(0x38);
    LCD_command(0x06);
    LCD_command(0x01);
    LCD_command(0x0F);
}

void LCD_command(unsigned char command)
{
    PTA->PCOR = RS | RW;
    PTD->PDOR = command;
    PTA->PSOR = EN;
    delayMs(1);
    PTA->PCOR = EN;
}

void LCD_data(unsigned char data)
{
    PTA->PSOR = RS;
    PTA->PCOR = RW;
    PTD->PDOR = data;
    PTA->PSOR = EN;
    delayMs(1);
    PTA->PCOR = EN;
}

void LCD_sendstring(char *str)
{
    while (*str) LCD_data(*str++);
}

//DELAY
void delayMs(int n)
{
    for(int i=0;i<n;i++)
        for(int j=0;j<3000;j++)
            __asm("nop");
}

void delayUs(int n)
{
    for(int i=0;i<n*50;i++)
        __asm("nop");
}

