#include "MKL25Z4.h"
#include <stdint.h>
#include <stdbool.h>
#include "matt_write_config.h"

#ifndef GPIOA
#define GPIOA PTA
#define GPIOB PTB
#define GPIOC PTC
#define GPIOD PTD
#define GPIOE PTE
#endif

// =======================================================
// TEST/CALIBRACION X-DRIVE - mismo cableado que mat_mamalon_final.c
//
// Ruedas fisicas:
//   FL: /
//   FR: \.
//   RR: /
//   RL: \.
//
// Mapa:
//   FL = MOTOR_1
//   FR = MOTOR_2
//   RR = MOTOR_3
//   RL = MOTOR_4
//
// Cableado:
//   MOTOR_1/FL: Right PWM PTC8/TPM0_CH4,  Left PWM PTC9/TPM0_CH5
//   MOTOR_2/FR: Right PWM PTB1/TPM1_CH1,  Left PWM PTB0/TPM1_CH0
//   MOTOR_3/RR: Right PWM PTB2/TPM2_CH0,  Left PWM PTB3/TPM2_CH1
//   MOTOR_4/RL: Right PWM PTE29/TPM0_CH2, Left PWM PTC2/TPM0_CH1
//
// Compila/carga SOLO este archivo para probar movimiento. Excluye
// mat_mamalon_final.c del build mientras uses esta rutina.
// =======================================================

#define MOTOR_1 0u
#define MOTOR_2 1u
#define MOTOR_3 2u
#define MOTOR_4 3u

#define FL MOTOR_1
#define FR MOTOR_2
#define RR MOTOR_3
#define RL MOTOR_4

#define DIR_STOP     0u
#define DIR_FORWARD  1u
#define DIR_BACKWARD 2u
#define DIR_BRAKE    3u

#define PWM_PERIOD 300u
#define TEST_PAUSE_MS 900u

// Segun la prueba con encoders, avanzar recto usa:
// M1 FORWARD, M2 BACKWARD, M3 FORWARD, M4 BACKWARD.
// Por eso FR/M2 y RL/M4 quedan invertidos aqui.
#define FL_DIR_SIGN 1
#define FR_DIR_SIGN -1
#define RL_DIR_SIGN -1
#define RR_DIR_SIGN 1

// Factores de calibracion por motor.
// Baja ligeramente el motor que empuja mas fuerte.
float calFL = 1.00f;
float calFR = 1.00f;
float calRL = 1.00f;
float calRR = 1.00f;

// Guia rapida:
// - Si al mover arriba el robot se va hacia la derecha, reduce ligeramente
//   los motores que causan ese desvio.
// - Si al mover arriba el robot gira, revisa si un motor esta invertido.
// - Si al mover derecha se va en diagonal, ajusta calFL, calFR, calRL y calRR.
// - Si un motor empuja al sentido contrario, invierte su direccion con
//   FL_DIR_SIGN/FR_DIR_SIGN/RL_DIR_SIGN/RR_DIR_SIGN o cambia el signo de ese motor.
//
// Ejemplo:
//   float calFL = 1.00f;
//   float calFR = 0.92f;
//   float calRL = 1.00f;
//   float calRR = 0.95f;

static volatile uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++;
}

static void delay_ms(uint32_t ms) {
    uint32_t start = msTicks;
    while ((msTicks - start) < ms) {}
}

static int16_t clampSpeed(int16_t value) {
    if (value > (int16_t)MOTOR_SPEED_MAX) return (int16_t)MOTOR_SPEED_MAX;
    if (value < -(int16_t)MOTOR_SPEED_MAX) return -(int16_t)MOTOR_SPEED_MAX;
    return value;
}

static uint8_t absSpeed(int16_t value) {
    if (value < 0) value = (int16_t)-value;
    if (value > (int16_t)MOTOR_SPEED_MAX) value = (int16_t)MOTOR_SPEED_MAX;
    return (uint8_t)value;
}

static int16_t applyCalibration(int16_t speed, float calibration) {
    float scaled = (float)speed * calibration;

    if (scaled >= 0.0f) {
        return clampSpeed((int16_t)(scaled + 0.5f));
    }

    return clampSpeed((int16_t)(scaled - 0.5f));
}

static int8_t motorDirectionSign(uint8_t motor) {
    switch (motor) {
        case FL: return FL_DIR_SIGN;
        case FR: return FR_DIR_SIGN;
        case RL: return RL_DIR_SIGN;
        case RR: return RR_DIR_SIGN;
        default: return 1;
    }
}

static void setup_tpm_all_channels(TPM_Type *tpm, uint8_t maxChannels) {
    tpm->SC  = 0;
    tpm->CNT = 0;
    tpm->MOD = PWM_PERIOD - 1u;

    for (uint8_t ch = 0; ch < maxChannels; ch++) {
        tpm->CONTROLS[ch].CnSC = TPM_CnSC_MSB_MASK | TPM_CnSC_ELSB_MASK;
        tpm->CONTROLS[ch].CnV  = 0;
    }

    tpm->SC = TPM_SC_CMOD(1) | TPM_SC_PS(3);
}

static void tpm_motor_init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK |
                  SIM_SCGC5_PORTB_MASK |
                  SIM_SCGC5_PORTC_MASK |
                  SIM_SCGC5_PORTE_MASK;

    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK |
                  SIM_SCGC6_TPM1_MASK |
                  SIM_SCGC6_TPM2_MASK;

    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(1);

    PORTC->PCR[8]  = PORT_PCR_MUX(3); // FL Right PWM PTC8  TPM0_CH4
    PORTC->PCR[9]  = PORT_PCR_MUX(3); // FL Left PWM  PTC9  TPM0_CH5
    PORTB->PCR[1]  = PORT_PCR_MUX(3); // FR Right PWM PTB1  TPM1_CH1
    PORTB->PCR[0]  = PORT_PCR_MUX(3); // FR Left PWM  PTB0  TPM1_CH0
    PORTB->PCR[2]  = PORT_PCR_MUX(3); // RR Right PWM PTB2  TPM2_CH0
    PORTB->PCR[3]  = PORT_PCR_MUX(3); // RR Left PWM  PTB3  TPM2_CH1
    PORTE->PCR[29] = PORT_PCR_MUX(3); // RL Right PWM PTE29 TPM0_CH2
    PORTC->PCR[2]  = PORT_PCR_MUX(4); // RL Left PWM  PTC2  TPM0_CH1

    setup_tpm_all_channels(TPM0, 6);
    setup_tpm_all_channels(TPM1, 2);
    setup_tpm_all_channels(TPM2, 2);
}

static void motor_set_raw(uint8_t motor, uint8_t dir, uint8_t speed) {
    if (speed > MOTOR_SPEED_MAX) speed = MOTOR_SPEED_MAX;

    uint32_t duty = (uint32_t)speed * (PWM_PERIOD - 1u) / 100u;
    uint32_t rightPwm = 0u;
    uint32_t leftPwm = 0u;

    if (dir == DIR_FORWARD && speed > 0u) {
        rightPwm = duty;
    } else if (dir == DIR_BACKWARD && speed > 0u) {
        leftPwm = duty;
    } else if (dir == DIR_BRAKE) {
        rightPwm = PWM_PERIOD - 1u;
        leftPwm = PWM_PERIOD - 1u;
    }

    switch (motor) {
        case FL:
            TPM0->CONTROLS[4].CnV = rightPwm;
            TPM0->CONTROLS[5].CnV = leftPwm;
            break;

        case FR:
            TPM1->CONTROLS[1].CnV = rightPwm;
            TPM1->CONTROLS[0].CnV = leftPwm;
            break;

        case RR:
            TPM2->CONTROLS[0].CnV = rightPwm;
            TPM2->CONTROLS[1].CnV = leftPwm;
            break;

        case RL:
            TPM0->CONTROLS[2].CnV = rightPwm;
            TPM0->CONTROLS[1].CnV = leftPwm;
            break;
    }
}

// speed: -70 a 70. Positivo = adelante, negativo = atras, 0 = stop.
static void setMotor(uint8_t motor, int16_t speed) {
    speed = clampSpeed(speed);
    speed = (int16_t)(speed * motorDirectionSign(motor));

    if (speed > 0) {
        motor_set_raw(motor, DIR_FORWARD, absSpeed(speed));
    } else if (speed < 0) {
        motor_set_raw(motor, DIR_BACKWARD, absSpeed(speed));
    } else {
        motor_set_raw(motor, DIR_STOP, 0u);
    }
}

static void stopMotors(void) {
    setMotor(FL, 0);
    setMotor(FR, 0);
    setMotor(RL, 0);
    setMotor(RR, 0);

    TPM0->CONTROLS[4].CnV = 0;
    TPM0->CONTROLS[5].CnV = 0;
    TPM0->CONTROLS[1].CnV = 0;
    TPM0->CONTROLS[2].CnV = 0;
    TPM1->CONTROLS[0].CnV = 0;
    TPM1->CONTROLS[1].CnV = 0;
    TPM2->CONTROLS[0].CnV = 0;
    TPM2->CONTROLS[1].CnV = 0;
}

static void testMotor(uint8_t motor, int16_t speed, uint32_t durationMs) {
    stopMotors();
    setMotor(motor, speed);
    delay_ms(durationMs);
    stopMotors();
}

// x controla izquierda/derecha. y controla arriba/abajo.
// Ambos entran de -70 a 70.
static void moveXY(int16_t x, int16_t y) {
    int16_t fl;
    int16_t fr;
    int16_t rl;
    int16_t rr;
    int16_t maxAbs;

    x = clampSpeed(x);
    y = clampSpeed(y);

    fl = (int16_t)(y + x);
    fr = (int16_t)(y - x);
    rl = (int16_t)(y - x);
    rr = (int16_t)(y + x);

    maxAbs = absSpeed(fl);
    if ((int16_t)absSpeed(fr) > maxAbs) maxAbs = absSpeed(fr);
    if ((int16_t)absSpeed(rl) > maxAbs) maxAbs = absSpeed(rl);
    if ((int16_t)absSpeed(rr) > maxAbs) maxAbs = absSpeed(rr);

    if (maxAbs > (int16_t)MOTOR_SPEED_MAX) {
        fl = (int16_t)((int32_t)fl * MOTOR_SPEED_MAX / maxAbs);
        fr = (int16_t)((int32_t)fr * MOTOR_SPEED_MAX / maxAbs);
        rl = (int16_t)((int32_t)rl * MOTOR_SPEED_MAX / maxAbs);
        rr = (int16_t)((int32_t)rr * MOTOR_SPEED_MAX / maxAbs);
    }

    fl = applyCalibration(fl, calFL);
    fr = applyCalibration(fr, calFR);
    rl = applyCalibration(rl, calRL);
    rr = applyCalibration(rr, calRR);

    setMotor(FL, fl);
    setMotor(FR, fr);
    setMotor(RL, rl);
    setMotor(RR, rr);
}

static void moveUp(void) {
    moveXY(0, MOTOR_SPEED_MAX);
}

static void moveDown(void) {
    moveXY(0, -(int16_t)MOTOR_SPEED_MAX);
}

static void moveRight(void) {
    moveXY(MOTOR_SPEED_MAX, 0);
}

static void moveLeft(void) {
    moveXY(-(int16_t)MOTOR_SPEED_MAX, 0);
}

static void moveUpTest(uint32_t durationMs) {
    moveUp();
    delay_ms(durationMs);
    stopMotors();
}

static void moveRightTest(uint32_t durationMs) {
    moveRight();
    delay_ms(durationMs);
    stopMotors();
}

int main(void) {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    tpm_motor_init();
    stopMotors();
    delay_ms(1500u);

    while (1) {
        // Paso 1: prueba individual. Revisa que todos giren y que + sea consistente.
        testMotor(FL, 40, 1000u);
        delay_ms(TEST_PAUSE_MS);
        testMotor(FR, 40, 1000u);
        delay_ms(TEST_PAUSE_MS);
        testMotor(RR, 40, 1000u);
        delay_ms(TEST_PAUSE_MS);
        testMotor(RL, 40, 1000u);
        delay_ms(TEST_PAUSE_MS);

        // Paso 2: primero calibra solo subida recta.
        moveUpTest(1000u);
        delay_ms(TEST_PAUSE_MS);

        // Paso 3: cuando la subida ya este recta, habilita esta prueba lateral.
        // moveRightTest(1000u);
        // delay_ms(TEST_PAUSE_MS);
    }
}
