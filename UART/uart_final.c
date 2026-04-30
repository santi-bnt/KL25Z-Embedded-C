#include "MKL25Z4.h"
#include <stdio.h>
#include <stdint.h>

//=========================
// PINES
//=========================

// Sensor ultrasónico en PORTB
#define TRIG 0x01   // PTB0
#define ECHO 0x02   // PTB1

// Stepper en PORTD
// IN1 -> PTD0
// IN2 -> PTD1
// IN3 -> PTD2
// IN4 -> PTD3

// UART hacia computadora
// UART0_TX -> PTA2

//=========================
// VARIABLES
//=========================

volatile float distancia = 0.0f;
volatile int angulo = 0;

//=========================
// PROTOTIPOS
//=========================

void UART0_init(void);
void UART0_putchar(char c);
void UART0_puts(char *s);

void ultrasonic_init(void);
float medir_distancia(void);

void motor_init(void);
void stepper_step(void);

void delayUs(int n);
void delayMs(int n);

//=========================
// MAIN
//=========================

int main(void)
{
    char buffer[40];
    int distancia_entera;
    int contador = 0;

    UART0_init();
    ultrasonic_init();
    motor_init();

    while(1)
    {
        // Mover motor un paso
        stepper_step();

        contador++;

        /*
         * No medimos distancia en cada paso porque el ultrasónico tarda.
         * Medimos cada 4 pasos para no trabar tanto el motor.
         */
        if(contador >= 4)
        {
            contador = 0;

            distancia = medir_distancia();
            distancia_entera = (int)(distancia + 0.5f);

            /*
             * Formato para Python:
             * angulo,distancia
             */
            sprintf(buffer, "%d,%d\r\n", angulo, distancia_entera);
            UART0_puts(buffer);
        }

        delayMs(3);
    }
}

//=========================
// UART0 TX HACIA PYTHON
//=========================

void UART0_init(void)
{
    SIM->SCGC4 |= 0x0400;    // Clock UART0
    SIM->SCGC5 |= 0x0200;    // Clock PORTA

    UART0->C2 = 0x00;

    SIM->SOPT2 &= ~0x0C000000;
    SIM->SOPT2 |= 0x04000000;

    /*
     * En tu caso Python lo lee bien con BAUD_RATE = 57600.
     */
    UART0->BDH = 0x00;
    UART0->BDL = 0x17;
    UART0->C4  = 0x0F;

    UART0->C1 = 0x00;

    // Solo TX, para no usar PTA1
    PORTA->PCR[2] = 0x0200;   // PTA2 = UART0_TX

    UART0->C2 = 0x08;         // Habilitar transmisión
}

void UART0_putchar(char c)
{
    while (!(UART0->S1 & 0x80))
    {
    }

    UART0->D = c;
}

void UART0_puts(char *s)
{
    while (*s)
    {
        UART0_putchar(*s++);
    }
}

//=========================
// MOTOR STEPPER
//=========================

void motor_init(void)
{
    SIM->SCGC5 |= 0x1000;    // Clock PORTD

    PORTD->PCR[0] = 0x100;   // PTD0 GPIO
    PORTD->PCR[1] = 0x100;   // PTD1 GPIO
    PORTD->PCR[2] = 0x100;   // PTD2 GPIO
    PORTD->PCR[3] = 0x100;   // PTD3 GPIO

    PTD->PDDR |= 0x0F;       // PTD0-PTD3 salidas
    PTD->PCOR = 0x0F;        // Apagar salidas al inicio
}

void stepper_step(void)
{
    static int paso = 0;
    static int pasos_totales = 0;
    static int dir = 1;

    const uint8_t seq[4] =
    {
        0x01,
        0x02,
        0x04,
        0x08
    };

    PTD->PCOR = 0x0F;
    PTD->PSOR = seq[paso];

    paso += dir;

    if(paso > 3)
    {
        paso = 0;
    }

    if(paso < 0)
    {
        paso = 3;
    }

    pasos_totales += dir;

    if(pasos_totales >= 2048)
    {
        pasos_totales = 2048;
        dir = -1;
    }

    if(pasos_totales <= 0)
    {
        pasos_totales = 0;
        dir = 1;
    }

    angulo = (pasos_totales * 180) / 2048;
}

//=========================
// SENSOR ULTRASONICO
//=========================

void ultrasonic_init(void)
{
    SIM->SCGC5 |= 0x0400;    // Clock PORTB

    PORTB->PCR[0] = 0x100;   // PTB0 GPIO TRIG
    PORTB->PCR[1] = 0x100;   // PTB1 GPIO ECHO

    PTB->PDDR |= TRIG;       // TRIG salida
    PTB->PDDR &= ~ECHO;      // ECHO entrada

    PTB->PCOR = TRIG;
}

float medir_distancia(void)
{
    uint32_t tiempo = 0;
    uint32_t timeout = 0;

    PTB->PCOR = TRIG;
    delayUs(2);

    PTB->PSOR = TRIG;
    delayUs(10);
    PTB->PCOR = TRIG;

    // Esperar a que ECHO suba
    timeout = 0;

    while(!(PTB->PDIR & ECHO))
    {
        timeout++;

        if(timeout > 30000)
        {
            return 0.0f;
        }

        delayUs(1);
    }

    // Medir cuánto tiempo ECHO está en alto
    tiempo = 0;

    while(PTB->PDIR & ECHO)
    {
        tiempo++;

        if(tiempo > 30000)
        {
            return 0.0f;
        }

        delayUs(1);
    }

    return (tiempo * 0.0343f) / 2.0f;
}

//=========================
// DELAYS
//=========================

void delayUs(int n)
{
    int i;

    for(i = 0; i < n * 50; i++)
    {
        __asm("nop");
    }
}

void delayMs(int n)
{
    int i;
    int j;

    for(i = 0; i < n; i++)
    {
        for(j = 0; j < 7000; j++)
        {
        }
    }
}
