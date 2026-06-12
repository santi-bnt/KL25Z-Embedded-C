#include "MKL25Z4.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "matt_write_config.h"

#ifndef GPIOA
#define GPIOA PTA
#define GPIOB PTB
#define GPIOC PTC
#define GPIOD PTD
#define GPIOE PTE
#endif

// =======================================================
// CONFIGURACION GENERAL
// =======================================================
#define UART_BAUD              9600
#define MANUAL_BUFFER_SIZE     33
#define UART_TEXT_BUFFER_SIZE  64
#define UART_LINE_BUFFER_SIZE  160
#define FINAL_TEXT_BUFFER_SIZE 160
#define MANUAL_START_DELAY_MS  3000
#define UART_RX_TIMEOUT_MS     6000u
#define UART_RX_DISPLAY_MS     350u

volatile uint32_t msTicks = 0;

void SysTick_Handler(void) {
    msTicks++;
}

void delay_ms(uint32_t ms) {
    uint32_t inicio = msTicks;
    while ((msTicks - inicio) < ms) {}
}

void delay_us(uint32_t us) {
    uint32_t cyclesPerUs = SystemCoreClock / 1000000u;
    volatile uint32_t cycles = us * (cyclesPerUs / 5u);
    while (cycles--) {}
}

// =======================================================
// LCD 16x2 EN MODO 4 BITS
// RS -> PTD4
// RW -> PTA4
// E  -> PTA5
// D4 -> PTD0
// D5 -> PTD1
// D6 -> PTD2
// D7 -> PTD3
// =======================================================
#define LCD_RS_PORT GPIOD
#define LCD_RS_PIN  4

#define LCD_RW_PORT GPIOA
#define LCD_RW_PIN  4

#define LCD_E_PORT  GPIOA
#define LCD_E_PIN   5

#define LCD_D4_PORT GPIOD
#define LCD_D4_PIN  0
#define LCD_D5_PORT GPIOD
#define LCD_D5_PIN  1
#define LCD_D6_PORT GPIOD
#define LCD_D6_PIN  2
#define LCD_D7_PORT GPIOD
#define LCD_D7_PIN  3

void pin_on(GPIO_Type *port, uint32_t pin) {
    port->PSOR = (1u << pin);
}

void pin_off(GPIO_Type *port, uint32_t pin) {
    port->PCOR = (1u << pin);
}

void lcd_set_data_pins(uint8_t nibble) {
    (nibble & 0x01) ? pin_on(LCD_D4_PORT, LCD_D4_PIN) : pin_off(LCD_D4_PORT, LCD_D4_PIN);
    (nibble & 0x02) ? pin_on(LCD_D5_PORT, LCD_D5_PIN) : pin_off(LCD_D5_PORT, LCD_D5_PIN);
    (nibble & 0x04) ? pin_on(LCD_D6_PORT, LCD_D6_PIN) : pin_off(LCD_D6_PORT, LCD_D6_PIN);
    (nibble & 0x08) ? pin_on(LCD_D7_PORT, LCD_D7_PIN) : pin_off(LCD_D7_PORT, LCD_D7_PIN);
}

void lcd_pulse_enable(void) {
    pin_on(LCD_E_PORT, LCD_E_PIN);
    delay_ms(1);
    pin_off(LCD_E_PORT, LCD_E_PIN);
    delay_ms(1);
}

void lcd_send(uint8_t value, uint8_t rs) {
    pin_off(LCD_RW_PORT, LCD_RW_PIN);

    if (rs) pin_on(LCD_RS_PORT, LCD_RS_PIN);
    else    pin_off(LCD_RS_PORT, LCD_RS_PIN);

    lcd_set_data_pins(value >> 4);
    lcd_pulse_enable();

    lcd_set_data_pins(value & 0x0F);
    lcd_pulse_enable();

    delay_ms(2);
}

void lcd_command(uint8_t cmd) { lcd_send(cmd, 0); }
void lcd_data(uint8_t data)   { lcd_send(data, 1); }

void lcd_clear(void) {
    lcd_command(0x01);
    delay_ms(3);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? (0x00 + col) : (0x40 + col);
    lcd_command(0x80 | address);
}

void lcd_print(const char *text) {
    while (*text) lcd_data((uint8_t)*text++);
}

void lcd_print_padded(const char *text) {
    uint8_t i = 0;
    while (text[i] && i < 16) { lcd_data((uint8_t)text[i]); i++; }
    while (i < 16)             { lcd_data(' '); i++; }
}

static void lcd_print_rx_tail(const char *text) {
    char linea[17];
    uint8_t len = (uint8_t)strlen(text);

    if (len > 13u) text = &text[len - 13u];
    snprintf(linea, sizeof(linea), "RX:%s", text);
    lcd_print_padded(linea);
}

void lcd_init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTD_MASK;

    PORTD->PCR[LCD_RS_PIN] = PORT_PCR_MUX(1);
    PORTA->PCR[LCD_RW_PIN] = PORT_PCR_MUX(1);
    PORTA->PCR[LCD_E_PIN]  = PORT_PCR_MUX(1);
    PORTD->PCR[LCD_D4_PIN] = PORT_PCR_MUX(1);
    PORTD->PCR[LCD_D5_PIN] = PORT_PCR_MUX(1);
    PORTD->PCR[LCD_D6_PIN] = PORT_PCR_MUX(1);
    PORTD->PCR[LCD_D7_PIN] = PORT_PCR_MUX(1);

    GPIOD->PDDR |= (1u << LCD_RS_PIN) | (1u << LCD_D4_PIN) |
                   (1u << LCD_D5_PIN) | (1u << LCD_D6_PIN) | (1u << LCD_D7_PIN);
    GPIOA->PDDR |= (1u << LCD_RW_PIN) | (1u << LCD_E_PIN);

    delay_ms(50);
    pin_off(LCD_RS_PORT, LCD_RS_PIN);
    pin_off(LCD_RW_PORT, LCD_RW_PIN);
    pin_off(LCD_E_PORT,  LCD_E_PIN);

    lcd_set_data_pins(0x03); lcd_pulse_enable(); delay_ms(5);
    lcd_set_data_pins(0x03); lcd_pulse_enable(); delay_ms(5);
    lcd_set_data_pins(0x03); lcd_pulse_enable(); delay_ms(5);
    lcd_set_data_pins(0x02); lcd_pulse_enable(); delay_ms(5);

    lcd_command(0x28);
    lcd_command(0x0C);
    lcd_command(0x06);
    lcd_clear();
}

// =======================================================
// UART1 EN PTE0/PTE1
// PTE1 = UART1_RX  <- ESP32 TX GPIO18
// PTE0 = UART1_TX  -> ESP32 RX GPIO19
// =======================================================
void uart0_init(void) {
    uint16_t sbr;
    SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
    SIM->SCGC4 |= SIM_SCGC4_UART1_MASK;

    PORTE->PCR[0] = PORT_PCR_MUX(3);
    PORTE->PCR[1] = PORT_PCR_MUX(3);

    UART1->C2 &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);

    uint32_t busClock = SystemCoreClock / 2u;
    sbr = (uint16_t)(busClock / (UART_BAUD * 16u));

    UART1->BDH = (UART1->BDH & ~UART_BDH_SBR_MASK) |
                 ((sbr >> 8) & UART_BDH_SBR_MASK);
    UART1->BDL = (uint8_t)(sbr & 0xFF);
    UART1->C1  = 0x00;
    UART1->C4  = 0x0F;
    UART1->C2 |= UART_C2_TE_MASK | UART_C2_RE_MASK;
}

bool uart0_available(void) {
    return (UART1->S1 & UART_S1_RDRF_MASK) != 0;
}

char uart0_getchar_blocking(void) {
    while (!uart0_available()) {}
    return (char)UART1->D;
}

void uart0_putchar(char c) {
    while (!(UART1->S1 & UART_S1_TDRE_MASK)) {}
    UART1->D = (uint8_t)c;
}

void uart0_print(const char *text) {
    while (*text) uart0_putchar(*text++);
}

static void kl_status(const char *linea1, const char *linea2) {
    uart0_print("#STATUS:");
    uart0_print(linea1);
    uart0_print("|");
    uart0_print(linea2);
    uart0_print("\n");
}

// =======================================================
// KEYPAD 4x4
// Filas:    PTC0, PTC1, PTB8, PTC3
// PTC2 queda reservado para Driver 4 Left PWM por TPM0_CH1.
// Columnas: PTC4, PTC5, PTC6, PTC7
// =======================================================
#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

static PORT_Type * const rowPortCfg[KEYPAD_ROWS] = {PORTC, PORTC, PORTB, PORTC};
static GPIO_Type * const rowGpio[KEYPAD_ROWS] = {GPIOC, GPIOC, GPIOB, GPIOC};
static const uint8_t rowPins[KEYPAD_ROWS] = {0, 1, 8, 3};
static const uint8_t colPins[KEYPAD_COLS] = {4, 5, 6, 7};

static const char keymap[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

void keypad_init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK;

    for (uint8_t r = 0; r < KEYPAD_ROWS; r++) {
        rowPortCfg[r]->PCR[rowPins[r]] = PORT_PCR_MUX(1);
        rowGpio[r]->PDDR |= (1u << rowPins[r]);
        rowGpio[r]->PSOR  = (1u << rowPins[r]);
    }

    for (uint8_t c = 0; c < KEYPAD_COLS; c++) {
        PORTC->PCR[colPins[c]] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
        GPIOC->PDDR &= ~(1u << colPins[c]);
    }
}

char keypad_getkey(void) {
    for (uint8_t r = 0; r < KEYPAD_ROWS; r++) {
        for (uint8_t i = 0; i < KEYPAD_ROWS; i++) {
            rowGpio[i]->PSOR = (1u << rowPins[i]);
        }
        rowGpio[r]->PCOR = (1u << rowPins[r]);
        delay_ms(1);

        for (uint8_t c = 0; c < KEYPAD_COLS; c++) {
            if ((GPIOC->PDIR & (1u << colPins[c])) == 0) {
                delay_ms(30);
                if ((GPIOC->PDIR & (1u << colPins[c])) == 0) {
                    while ((GPIOC->PDIR & (1u << colPins[c])) == 0) delay_ms(5);
                    return keymap[r][c];
                }
            }
        }
    }
    return 0;
}

// =======================================================
// UTILIDADES DE TEXTO / CALCULADORA SIMPLE
// =======================================================
bool es_digito(char c) { return (c >= '0' && c <= '9'); }

const char *saltar_espacios(const char *p) {
    while (*p == ' ') p++;
    return p;
}

bool parse_int32(const char **pp, int32_t *valor) {
    const char *p = saltar_espacios(*pp);
    int signo = 1;
    int32_t n = 0;
    bool hayDigito = false;

    if (*p == '-') { signo = -1; p++; }
    while (es_digito(*p)) { hayDigito = true; n = (n * 10) + (*p - '0'); p++; }
    if (!hayDigito) return false;
    *valor = n * signo;
    *pp = p;
    return true;
}

int32_t parse_mm_value(const char **pp) {
    const char *p = saltar_espacios(*pp);
    int32_t n = 0, signo = 1;
    if (*p == '-') { signo = -1; p++; }
    while (es_digito(*p)) { n = n * 10 + (*p - '0'); p++; }
    if (*p == '.') { p++; while (es_digito(*p)) p++; }
    *pp = p;
    return n * signo;
}

bool evaluar_operacion_simple(const char *expr, char *resultado, uint8_t resultadoSize) {
    const char *p = expr;
    int32_t a, b;
    char op;

    p = saltar_espacios(p);
    if (!parse_int32(&p, &a)) return false;
    p = saltar_espacios(p);
    op = *p;
    if (op != '+' && op != '-' && op != '*' && op != '/') return false;
    p++;
    if (!parse_int32(&p, &b)) return false;
    p = saltar_espacios(p);
    if (*p != '=') return false;
    p++;
    p = saltar_espacios(p);
    if (*p != '\0') return false;

    if (op == '+') snprintf(resultado, resultadoSize, "%ld", (long)(a + b));
    else if (op == '-') snprintf(resultado, resultadoSize, "%ld", (long)(a - b));
    else if (op == '*') snprintf(resultado, resultadoSize, "%ld", (long)(a * b));
    else if (op == '/') {
        if (b == 0) {
            snprintf(resultado, resultadoSize, "ERROR");
        } else if ((a % b) == 0) {
            snprintf(resultado, resultadoSize, "%ld", (long)(a / b));
        } else {
            int32_t entero = a / b;
            int32_t residuo = a % b;
            if (residuo < 0) residuo = -residuo;
            int32_t decimales = (residuo * 100) / (b < 0 ? -b : b);
            snprintf(resultado, resultadoSize, "%ld.%02ld", (long)entero, (long)decimales);
        }
    }
    return true;
}

void copiar_seguro(char *destino, const char *origen, uint8_t maxSize) {
    uint8_t i = 0;
    if (maxSize == 0) return;
    while (origen[i] && i < maxSize - 1) { destino[i] = origen[i]; i++; }
    destino[i] = '\0';
}

void procesar_texto_antes_de_escribir(const char *entrada, char *salida, uint8_t salidaSize) {
    char resultado[20];
    if (evaluar_operacion_simple(entrada, resultado, sizeof(resultado)))
        snprintf(salida, salidaSize, "%s%s", entrada, resultado);
    else
        copiar_seguro(salida, entrada, salidaSize);
}

// =======================================================
// MOTOR DRIVER - BTS7960 43A - 4 DRIVERS
//
// Conexiones BTS7960 solicitadas: Right PWM + Left PWM.
// Los enables del BTS7960 van directo a VCC.
//
// Driver 1:
//   Right PWM -> PTC8 -> TPM0_CH4
//   Left PWM  -> PTC9 -> TPM0_CH5
//
// Driver 2:
//   Right PWM -> PTB1 -> TPM1_CH1
//   Left PWM  -> PTB0 -> TPM1_CH0
//
// Driver 3:
//   Right PWM -> PTB2 -> TPM2_CH0
//   Left PWM  -> PTB3 -> TPM2_CH1
//
// Driver 4:
//   Right PWM -> PTE29 -> TPM0_CH2
//   Left PWM  -> PTC2  -> TPM0_CH1
//
// Nota: en KL25Z, PTE29 saca PWM por TPM0_CH2.
// Nota: PTC2 queda dedicado al Driver 4 y no se usa como fila del keypad.
//
// Direccion:
//   Adelante: Right PWM activo, Left PWM=0
//   Atras:    Right PWM=0,      Left PWM activo
//   Stop:     Right PWM=0,      Left PWM=0
//
// GND de todos los drivers -> GND comun con KL25Z
// VCC del driver -> VCC logico
// B+ y B- del BTS7960 -> fuente/bateria de motores
// =======================================================

#define MOTOR_1  0u
#define MOTOR_2  1u
#define MOTOR_3  2u
#define MOTOR_4  3u

// Alias para que tu codigo viejo siga funcionando
#define MOTOR_LEFT   MOTOR_1
#define MOTOR_RIGHT  MOTOR_2

#define DIR_STOP     0u
#define DIR_FORWARD  1u
#define DIR_BACKWARD 2u
#define DIR_BRAKE    3u

#define PWM_PERIOD   300u

static void motors_stop(void);
static void arm_stop(void);
void reset_uart_text_buffer(void);
void reset_uart_line_buffer(void);
static volatile bool stopRequested = false;

// =======================================================
// SERVOS DE ESCRITURA Y BORRADO
//
// Servo escritura -> PTD5
// Servo borrador  -> PTE23
//
// Calibracion:
//   UP   = levantado
//   DOWN = baja aprox. 70 grados
// =======================================================
static uint16_t servo_angle_to_us(uint8_t angle) {
    if (angle > 180u) angle = 180u;
    return (uint16_t)(1000u + ((uint32_t)angle * 1000u / 180u));
}

static void servo_pulse(GPIO_Type *gpio, uint32_t pin, uint8_t angle) {
    uint16_t highUs = servo_angle_to_us(angle);

    gpio->PSOR = (1u << pin);
    delay_us(highUs);
    gpio->PCOR = (1u << pin);
    delay_us(20000u - highUs);
}

static void servo_hold(GPIO_Type *gpio, uint32_t pin, uint8_t angle, uint32_t holdMs) {
    uint32_t cycles = holdMs / 20u;
    if (cycles == 0u) cycles = 1u;

    while (cycles--) {
        servo_pulse(gpio, pin, angle);
    }
}

static void servos_init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK | SIM_SCGC5_PORTE_MASK;

    SERVO_WRITE_PORT_CFG->PCR[SERVO_WRITE_PIN] = PORT_PCR_MUX(1);
    SERVO_ERASE_PORT_CFG->PCR[SERVO_ERASE_PIN] = PORT_PCR_MUX(1);

    SERVO_WRITE_GPIO->PDDR |= (1u << SERVO_WRITE_PIN);
    SERVO_ERASE_GPIO->PDDR |= (1u << SERVO_ERASE_PIN);

    SERVO_WRITE_GPIO->PCOR = (1u << SERVO_WRITE_PIN);
    SERVO_ERASE_GPIO->PCOR = (1u << SERVO_ERASE_PIN);

    servo_hold(SERVO_WRITE_GPIO, SERVO_WRITE_PIN, SERVO_UP_DEG, SERVO_HOLD_MS);
    servo_hold(SERVO_ERASE_GPIO, SERVO_ERASE_PIN, SERVO_ERASE_UP_DEG, SERVO_HOLD_MS);
}

static void write_servo_up(void) {
    servo_hold(SERVO_WRITE_GPIO, SERVO_WRITE_PIN, SERVO_UP_DEG, SERVO_HOLD_MS);
}

static void write_servo_down(void) {
    servo_hold(SERVO_WRITE_GPIO, SERVO_WRITE_PIN, SERVO_DOWN_DEG, SERVO_HOLD_MS);
}

static void erase_servo_up(void) {
    servo_hold(SERVO_ERASE_GPIO, SERVO_ERASE_PIN, SERVO_ERASE_UP_DEG, SERVO_HOLD_MS);
}

static void erase_servo_down(void) {
    servo_hold(SERVO_ERASE_GPIO, SERVO_ERASE_PIN, SERVO_ERASE_DOWN_DEG, SERVO_HOLD_MS);
}

static bool prioridad_stop_uart(void) {
    static char cmd[12];
    static uint8_t len = 0;

    while (uart0_available()) {
        char c = uart0_getchar_blocking();

        if (c == '\r') continue;

        if (c == '\n' || c == '#') {
            cmd[len] = '\0';
            len = 0;

            if (strcmp(cmd, "X") == 0 || strcmp(cmd, "M0") == 0 || strcmp(cmd, "STOP") == 0) {
                stopRequested = true;
                reset_uart_line_buffer();
                reset_uart_text_buffer();
                kl_status("STOP UART", "Parando todo");
                motors_stop();
                arm_stop();
                write_servo_up();
                uart0_print("OK\n");
                return true;
            }

            continue;
        }

        if (len < sizeof(cmd) - 1u) {
            cmd[len++] = c;
        } else {
            len = 0;
        }
    }

    return false;
}

static bool delay_ms_prioridad(uint32_t ms) {
    uint32_t inicio = msTicks;

    if (stopRequested) return true;

    while ((msTicks - inicio) < ms) {
        if (stopRequested || prioridad_stop_uart()) return true;
    }

    return false;
}

static bool delay_ms_prioridad_write_down(uint32_t ms) {
    uint32_t inicio = msTicks;

    if (stopRequested) return true;

    while ((msTicks - inicio) < ms) {
        servo_pulse(SERVO_WRITE_GPIO, SERVO_WRITE_PIN, SERVO_DOWN_DEG);
        if (stopRequested || prioridad_stop_uart()) return true;
    }

    return false;
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

static void encoder_inputs_init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTC_MASK;

    // Motor 1: Amarillo -> PTA1, Blanco -> PTA2
    PORTA->PCR[1] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTA->PCR[2] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    GPIOA->PDDR &= ~((1u << 1) | (1u << 2));

    // Motor 3: Amarillo -> PTA12, Blanco -> PTA13
    PORTA->PCR[12] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTA->PCR[13] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    GPIOA->PDDR &= ~((1u << 12) | (1u << 13));

    // Motor 2: Amarillo -> PTC10, Blanco -> PTC11
    PORTC->PCR[10] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTC->PCR[11] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    // Motor 4: Blanco -> PTC12, Amarillo -> PTC13
    PORTC->PCR[12] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    PORTC->PCR[13] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    GPIOC->PDDR &= ~((1u << 10) | (1u << 11) | (1u << 12) | (1u << 13));
}

static void tpm_motor_init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK |
                  SIM_SCGC5_PORTB_MASK |
                  SIM_SCGC5_PORTC_MASK |
                  SIM_SCGC5_PORTE_MASK;

    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK |
                  SIM_SCGC6_TPM1_MASK |
                  SIM_SCGC6_TPM2_MASK;

    // Fuente de clock TPM: MCGFLLCLK
    SIM->SOPT2 = (SIM->SOPT2 & ~SIM_SOPT2_TPMSRC_MASK) | SIM_SOPT2_TPMSRC(1);

    PORTC->PCR[8]  = PORT_PCR_MUX(3); // Driver 1 Right PWM PTC8  TPM0_CH4
    PORTC->PCR[9]  = PORT_PCR_MUX(3); // Driver 1 Left PWM  PTC9  TPM0_CH5
    PORTB->PCR[1]  = PORT_PCR_MUX(3); // Driver 2 Right PWM PTB1  TPM1_CH1
    PORTB->PCR[0]  = PORT_PCR_MUX(3); // Driver 2 Left PWM  PTB0  TPM1_CH0
    PORTB->PCR[2]  = PORT_PCR_MUX(3); // Driver 3 Right PWM PTB2  TPM2_CH0
    PORTB->PCR[3]  = PORT_PCR_MUX(3); // Driver 3 Left PWM  PTB3  TPM2_CH1
    PORTE->PCR[29] = PORT_PCR_MUX(3); // Driver 4 Right PWM PTE29 TPM0_CH2
    PORTC->PCR[2]  = PORT_PCR_MUX(4); // Driver 4 Left PWM  PTC2  TPM0_CH1

    setup_tpm_all_channels(TPM0, 6);
    setup_tpm_all_channels(TPM1, 2);
    setup_tpm_all_channels(TPM2, 2);

    encoder_inputs_init();
}

// motor: MOTOR_1 / MOTOR_2 / MOTOR_3 / MOTOR_4
// dir:   DIR_FORWARD / DIR_BACKWARD / DIR_STOP / DIR_BRAKE
// speed: 0-100
static void motor_set(uint8_t motor, uint8_t dir, uint8_t speed) {
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
        case MOTOR_1:
            TPM0->CONTROLS[4].CnV = rightPwm;  // PTC8 Right PWM
            TPM0->CONTROLS[5].CnV = leftPwm;   // PTC9 Left PWM
            break;

        case MOTOR_2:
            TPM1->CONTROLS[1].CnV = rightPwm;  // PTB1 Right PWM
            TPM1->CONTROLS[0].CnV = leftPwm;   // PTB0 Left PWM
            break;

        case MOTOR_3:
            TPM2->CONTROLS[0].CnV = rightPwm;  // PTB2 Right PWM
            TPM2->CONTROLS[1].CnV = leftPwm;   // PTB3 Left PWM
            break;

        case MOTOR_4:
            TPM0->CONTROLS[2].CnV = rightPwm;  // PTE29 Right PWM
            TPM0->CONTROLS[1].CnV = leftPwm;   // PTC2 Left PWM
            break;
    }
}

static void motor_stop_one(uint8_t motor) {
    motor_set(motor, DIR_STOP, 0);
}

static void motors_stop(void) {
    motor_set(MOTOR_1, DIR_STOP, 0);
    motor_set(MOTOR_2, DIR_STOP, 0);
    motor_set(MOTOR_3, DIR_STOP, 0);
    motor_set(MOTOR_4, DIR_STOP, 0);

    TPM0->CONTROLS[4].CnV = 0;
    TPM0->CONTROLS[5].CnV = 0;
    TPM0->CONTROLS[1].CnV = 0;
    TPM0->CONTROLS[2].CnV = 0;
    TPM1->CONTROLS[0].CnV = 0;
    TPM1->CONTROLS[1].CnV = 0;
    TPM2->CONTROLS[0].CnV = 0;
    TPM2->CONTROLS[1].CnV = 0;
}

static uint8_t motor_trim(uint8_t motor) {
    switch (motor) {
        case MOTOR_1: return MOTOR_1_TRIM;
        case MOTOR_2: return MOTOR_2_TRIM;
        case MOTOR_3: return MOTOR_3_TRIM;
        case MOTOR_4: return MOTOR_4_TRIM;
        default:      return 100u;
    }
}

static void motor_drive_cmd(uint8_t motor, int8_t cmd, uint8_t speed) {
    uint8_t trimmedSpeed;

    if (cmd == 0 || speed == 0u) {
        motor_set(motor, DIR_STOP, 0);
        return;
    }

    trimmedSpeed = (uint8_t)(((uint32_t)speed * motor_trim(motor)) / 100u);
    if (trimmedSpeed > MOTOR_SPEED_MAX) trimmedSpeed = MOTOR_SPEED_MAX;

    motor_set(motor, cmd > 0 ? DIR_FORWARD : DIR_BACKWARD, trimmedSpeed);
}

static void drive_pattern(int8_t m1, int8_t m2, int8_t m3, int8_t m4, uint8_t speed) {
    motor_drive_cmd(MOTOR_1, (int8_t)(m1 * MOTOR_1_SIGN), speed);
    motor_drive_cmd(MOTOR_2, (int8_t)(m2 * MOTOR_2_SIGN), speed);
    motor_drive_cmd(MOTOR_3, (int8_t)(m3 * MOTOR_3_SIGN), speed);
    motor_drive_cmd(MOTOR_4, (int8_t)(m4 * MOTOR_4_SIGN), speed);
}

// --- Movimientos de ruedas (con velocidad explicita) ---
static void move_forward_spd(uint8_t s) {
    drive_pattern(MOVE_FWD_M1, MOVE_FWD_M2, MOVE_FWD_M3, MOVE_FWD_M4, s);
}

static void move_backward_spd(uint8_t s) {
    drive_pattern(MOVE_BACK_M1, MOVE_BACK_M2, MOVE_BACK_M3, MOVE_BACK_M4, s);
}

// Ruedas a 45 grados tipo omni/mecanum:
// M1 = frontal izquierda, M2 = frontal derecha, M3 = trasera derecha, M4 = trasera izquierda.
// Para moverse lateralmente no se gira como carrito; se usa patron cruzado.
static void turn_left_spd(uint8_t s) {
    drive_pattern(MOVE_LEFT_M1, MOVE_LEFT_M2, MOVE_LEFT_M3, MOVE_LEFT_M4, s);
}

static void turn_right_spd(uint8_t s) {
    drive_pattern(MOVE_RIGHT_M1, MOVE_RIGHT_M2, MOVE_RIGHT_M3, MOVE_RIGHT_M4, s);
}

static void move_forward(void)  { move_forward_spd(DRIVE_SPEED); }
static void move_backward(void) { move_backward_spd(DRIVE_SPEED); }
static void turn_left(void)     { turn_left_spd(DRIVE_SPEED); }
static void turn_right(void)    { turn_right_spd(DRIVE_SPEED); }

static void fine_left(void) {
    turn_left_spd(FINE_SPEED);
}

static void fine_right(void) {
    turn_right_spd(FINE_SPEED);
}

// --- Brazo eraser con servo dedicado ---
static void arm_extend(void) {
    erase_servo_down();
}

static void arm_retract(void) {
    erase_servo_up();
}

static void arm_stop(void) {
    erase_servo_up();
}

// --- Barrido completo del tablero ---
static bool erase_board_sweep(void) {
    kl_status("Borrado total", "Extendiendo");
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Borrando...");
    lcd_set_cursor(1, 0);
    lcd_print("Extendiendo");

    arm_extend();
    if (delay_ms_prioridad(ARM_ENGAGE_MS)) return false;

    for (uint8_t row = 0; row < SWEEP_ROWS; row++) {
        char buf[17];
        snprintf(buf, sizeof(buf), "Carril %u/%u", (unsigned)(row + 1), (unsigned)SWEEP_ROWS);
        kl_status("Borrando carril", buf);
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Borrando...");
        lcd_set_cursor(1, 0);
        lcd_print_padded(buf);

        if ((row & 1u) == 0u) turn_right();
        else                  turn_left();
        if (delay_ms_prioridad(SWEEP_ROW_MS)) return false;
        motors_stop();
        if (delay_ms_prioridad(100)) return false;

        if (row < SWEEP_ROWS - 1u) {
            kl_status("Borrando", "Avanza carril");
            move_forward();
            if (delay_ms_prioridad(SWEEP_STEP_MS)) return false;
            motors_stop();
            if (delay_ms_prioridad(100)) return false;
        }
    }

    kl_status("Borrado total", "Retrayendo");
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Retrayendo");

    arm_retract();
    if (delay_ms_prioridad(ARM_ENGAGE_MS)) return false;

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Borrado listo");
    kl_status("Borrado listo", "OK");
    delay_ms(500);
    return true;
}

static bool navigate_to_with_write_pen(int32_t x_unit, int32_t y_unit, bool writePenDown) {
    // Con el plumon abajo se traza mas lento para que la linea salga continua;
    // el tiempo por unidad se escala para recorrer la misma distancia fisica.
    uint8_t  spd = writePenDown ? TRACE_SPEED : DRIVE_SPEED;
    uint32_t msH = writePenDown ? TRACE_MS_PER_UNIT_H : MS_PER_UNIT_H;
    uint32_t msV = writePenDown ? TRACE_MS_PER_UNIT_V : MS_PER_UNIT_V;

    if (x_unit > 0) {
        turn_right_spd(spd);
        if (writePenDown) {
            if (delay_ms_prioridad_write_down((uint32_t)x_unit * msH)) return false;
        } else {
            if (delay_ms_prioridad((uint32_t)x_unit * msH)) return false;
        }
        motors_stop();
        if (writePenDown) {
            if (delay_ms_prioridad_write_down(80)) return false;
        } else {
            if (delay_ms_prioridad(80)) return false;
        }
    } else if (x_unit < 0) {
        turn_left_spd(spd);
        if (writePenDown) {
            if (delay_ms_prioridad_write_down((uint32_t)(-x_unit) * msH)) return false;
        } else {
            if (delay_ms_prioridad((uint32_t)(-x_unit) * msH)) return false;
        }
        motors_stop();
        if (writePenDown) {
            if (delay_ms_prioridad_write_down(80)) return false;
        } else {
            if (delay_ms_prioridad(80)) return false;
        }
    }

    if (y_unit > 0) {
        move_backward_spd(spd);
        if (writePenDown) {
            if (delay_ms_prioridad_write_down((uint32_t)y_unit * msV)) return false;
        } else {
            if (delay_ms_prioridad((uint32_t)y_unit * msV)) return false;
        }
        motors_stop();
        if (writePenDown) {
            if (delay_ms_prioridad_write_down(80)) return false;
        } else {
            if (delay_ms_prioridad(80)) return false;
        }
    } else if (y_unit < 0) {
        move_forward_spd(spd);
        if (writePenDown) {
            if (delay_ms_prioridad_write_down((uint32_t)(-y_unit) * msV)) return false;
        } else {
            if (delay_ms_prioridad((uint32_t)(-y_unit) * msV)) return false;
        }
        motors_stop();
        if (writePenDown) {
            if (delay_ms_prioridad_write_down(80)) return false;
        } else {
            if (delay_ms_prioridad(80)) return false;
        }
    }

    return true;
}

static bool navigate_to(int32_t x_unit, int32_t y_unit) {
    return navigate_to_with_write_pen(x_unit, y_unit, false);
}

static bool trace_to(int32_t x_unit, int32_t y_unit) {
    return navigate_to_with_write_pen(x_unit, y_unit, true);
}

// =======================================================
// FSM_MEM PARA ESCRITURA DE LETRAS Y NUMEROS
// =======================================================
typedef struct {
    int8_t startX;
    int8_t startY;
    int8_t endX;
    int8_t endY;
} write_stroke_t;

typedef enum {
    WRITE_FSM_IDLE = 0,
    WRITE_FSM_MOVE_TO_START,
    WRITE_FSM_PEN_DOWN,
    WRITE_FSM_TRACE,
    WRITE_FSM_PEN_UP,
    WRITE_FSM_DONE,
    WRITE_FSM_ERROR
} write_fsm_state_t;

static write_stroke_t writeMemory[WRITE_MAX_STROKES];
static volatile uint8_t writeIndex = 0;
static volatile uint8_t currentStroke = 0;
static volatile write_fsm_state_t writeFsmState = WRITE_FSM_IDLE;

static void clear_write_memory(void) {
    writeIndex = 0;
    currentStroke = 0;
    writeFsmState = WRITE_FSM_IDLE;
}

static bool save_write_stroke(int8_t startX, int8_t startY, int8_t endX, int8_t endY) {
    if (writeIndex >= WRITE_MAX_STROKES) {
        writeFsmState = WRITE_FSM_ERROR;
        return false;
    }

    writeMemory[writeIndex].startX = (int8_t)(startX * WRITE_SCALE_UNITS);
    writeMemory[writeIndex].startY = (int8_t)(startY * WRITE_SCALE_UNITS);
    writeMemory[writeIndex].endX = (int8_t)(endX * WRITE_SCALE_UNITS);
    writeMemory[writeIndex].endY = (int8_t)(endY * WRITE_SCALE_UNITS);
    writeIndex++;
    return true;
}

static char to_upper_char(char c) {
    if (c >= 'a' && c <= 'z') return (char)(c - ('a' - 'A'));
    return c;
}

static bool glyph_rows(char c, uint8_t rows[GLYPH_HEIGHT]) {
    char symbol = to_upper_char(c);
    const uint8_t *src = MATT_GLYPH_BLANK;

    if (symbol != ' ') {
        bool found = false;

        for (uint8_t i = 0; i < MATT_GLYPH_COUNT; i++) {
            if (MATT_GLYPHS[i].symbol == symbol) {
                src = MATT_GLYPHS[i].rows;
                found = true;
                break;
            }
        }

        if (!found) return false;
    }

    for (uint8_t i = 0; i < GLYPH_HEIGHT; i++) rows[i] = src[i];
    return true;
}

static bool glyph_bit_on(uint8_t rowBits, uint8_t x) {
    return (rowBits & (1u << (GLYPH_WIDTH - 1u - x))) != 0u;
}

static bool save_row_strokes_ltr(uint8_t rowBits, uint8_t y) {
    uint8_t x = 0;

    while (x < GLYPH_WIDTH) {
        while (x < GLYPH_WIDTH && !glyph_bit_on(rowBits, x)) x++;
        if (x >= GLYPH_WIDTH) break;

        uint8_t startX = x;
        while (x < GLYPH_WIDTH && glyph_bit_on(rowBits, x)) x++;

        if (!save_write_stroke((int8_t)startX, (int8_t)y, (int8_t)x, (int8_t)y)) {
            return false;
        }
    }

    return true;
}

static bool save_row_strokes_rtl(uint8_t rowBits, uint8_t y) {
    int8_t x = (int8_t)GLYPH_WIDTH - 1;

    while (x >= 0) {
        while (x >= 0 && !glyph_bit_on(rowBits, (uint8_t)x)) x--;
        if (x < 0) break;

        int8_t startX = x + 1;
        while (x >= 0 && glyph_bit_on(rowBits, (uint8_t)x)) x--;

        if (!save_write_stroke(startX, (int8_t)y, (int8_t)(x + 1), (int8_t)y)) {
            return false;
        }
    }

    return true;
}

static bool save_column_strokes(uint8_t rows[GLYPH_HEIGHT],
                                bool covered[GLYPH_HEIGHT][GLYPH_WIDTH]) {
    for (uint8_t x = 0; x < GLYPH_WIDTH; x++) {
        uint8_t y = 0;

        while (y < GLYPH_HEIGHT) {
            while (y < GLYPH_HEIGHT && !glyph_bit_on(rows[y], x)) y++;
            if (y >= GLYPH_HEIGHT) break;

            uint8_t startY = y;
            while (y < GLYPH_HEIGHT && glyph_bit_on(rows[y], x)) y++;

            if ((uint8_t)(y - startY) >= 2u) {
                if (!save_write_stroke((int8_t)x, (int8_t)startY, (int8_t)x, (int8_t)y)) {
                    return false;
                }

                for (uint8_t yy = startY; yy < y; yy++) {
                    covered[yy][x] = true;
                }
            }
        }
    }

    return true;
}

static bool save_remaining_row_strokes_ltr(uint8_t rowBits, uint8_t y,
                                           bool covered[GLYPH_WIDTH]) {
    uint8_t x = 0;

    while (x < GLYPH_WIDTH) {
        while (x < GLYPH_WIDTH && (!glyph_bit_on(rowBits, x) || covered[x])) x++;
        if (x >= GLYPH_WIDTH) break;

        uint8_t startX = x;
        while (x < GLYPH_WIDTH && glyph_bit_on(rowBits, x) && !covered[x]) x++;

        if ((uint8_t)(x - startX) >= 2u) {
            if (!save_write_stroke((int8_t)startX, (int8_t)y, (int8_t)x, (int8_t)y)) {
                return false;
            }
        }
    }

    return true;
}

static bool save_remaining_row_strokes_rtl(uint8_t rowBits, uint8_t y,
                                           bool covered[GLYPH_WIDTH]) {
    int8_t x = (int8_t)GLYPH_WIDTH - 1;

    while (x >= 0) {
        while (x >= 0 && (!glyph_bit_on(rowBits, (uint8_t)x) || covered[(uint8_t)x])) x--;
        if (x < 0) break;

        int8_t startX = x + 1;
        while (x >= 0 && glyph_bit_on(rowBits, (uint8_t)x) && !covered[(uint8_t)x]) x--;

        if ((uint8_t)(startX - (x + 1)) >= 2u) {
            if (!save_write_stroke(startX, (int8_t)y, (int8_t)(x + 1), (int8_t)y)) {
                return false;
            }
        }
    }

    return true;
}

static bool save_original_row_strokes(uint8_t rows[GLYPH_HEIGHT]) {
    for (uint8_t y = 0; y < GLYPH_HEIGHT; y++) {
        if ((y & 1u) == 0u) {
            if (!save_row_strokes_ltr(rows[y], y)) return false;
        } else {
            if (!save_row_strokes_rtl(rows[y], y)) return false;
        }
    }

    return true;
}

static bool load_simple_line_char_to_write_memory(char c) {
    char ch = to_upper_char(c);

    if (ch == ' ') {
        writeFsmState = WRITE_FSM_DONE;
        return true;
    }

#define ST(x1, y1, x2, y2) do { \
    if (!save_write_stroke((int8_t)(x1), (int8_t)(y1), (int8_t)(x2), (int8_t)(y2))) return false; \
} while (0)

    switch (ch) {
        case '0':
        case 'O':
            ST(0,0,4,0); ST(4,0,4,7); ST(4,7,0,7); ST(0,7,0,0);
            break;
        case '1':
            ST(2,0,2,7); ST(1,7,3,7);
            break;
        case '2':
            ST(0,0,4,0); ST(4,0,4,3); ST(4,3,0,3); ST(0,3,0,7); ST(0,7,4,7);
            break;
        case '3':
            ST(0,0,4,0); ST(4,0,4,7); ST(0,3,4,3); ST(0,7,4,7);
            break;
        case '4':
            ST(0,0,0,3); ST(0,3,4,3); ST(4,0,4,7);
            break;
        case '5':
            ST(4,0,0,0); ST(0,0,0,3); ST(0,3,4,3); ST(4,3,4,7); ST(4,7,0,7);
            break;
        case '6':
            ST(4,0,0,0); ST(0,0,0,7); ST(0,7,4,7); ST(4,7,4,3); ST(4,3,0,3);
            break;
        case '7':
            ST(0,0,4,0); ST(4,0,4,7);
            break;
        case '8':
            ST(0,0,4,0); ST(4,0,4,7); ST(4,7,0,7); ST(0,7,0,0); ST(0,3,4,3);
            break;
        case '9':
            ST(4,7,4,0); ST(4,0,0,0); ST(0,0,0,3); ST(0,3,4,3); ST(4,7,0,7);
            break;
        case 'A':
            ST(0,7,0,0); ST(4,7,4,0); ST(0,0,4,0); ST(0,3,4,3);
            break;
        case 'B':
            ST(0,0,0,7); ST(0,0,4,0); ST(4,0,4,3); ST(4,3,0,3); ST(4,3,4,7); ST(4,7,0,7);
            break;
        case 'C':
            ST(4,0,0,0); ST(0,0,0,7); ST(0,7,4,7);
            break;
        case 'D':
            ST(0,0,0,7); ST(0,0,4,0); ST(4,0,4,7); ST(4,7,0,7);
            break;
        case 'E':
            ST(0,0,0,7); ST(0,0,4,0); ST(0,3,4,3); ST(0,7,4,7);
            break;
        case 'F':
            ST(0,0,0,7); ST(0,0,4,0); ST(0,3,4,3);
            break;
        case 'G':
            ST(4,0,0,0); ST(0,0,0,7); ST(0,7,4,7); ST(4,7,4,4); ST(4,4,2,4);
            break;
        case 'H':
            ST(0,0,0,7); ST(4,0,4,7); ST(0,3,4,3);
            break;
        case 'I':
            ST(0,0,4,0); ST(2,0,2,7); ST(0,7,4,7);
            break;
        case 'J':
            ST(0,0,4,0); ST(4,0,4,7); ST(4,7,1,7); ST(1,7,1,5);
            break;
        case 'K':
            ST(0,0,0,7); ST(0,0,4,0); ST(0,3,4,3); ST(0,7,4,7);
            break;
        case 'L':
            ST(0,0,0,7); ST(0,7,4,7);
            break;
        case 'M':
            ST(0,7,0,0); ST(4,7,4,0); ST(0,0,4,0); ST(2,0,2,3);
            break;
        case 'N':
            ST(0,7,0,0); ST(4,7,4,0); ST(0,0,4,0); ST(0,7,4,7);
            break;
        case 'P':
            ST(0,0,0,7); ST(0,0,4,0); ST(4,0,4,3); ST(4,3,0,3);
            break;
        case 'Q':
            ST(0,0,4,0); ST(4,0,4,7); ST(4,7,0,7); ST(0,7,0,0); ST(2,5,4,5);
            break;
        case 'R':
            ST(0,0,0,7); ST(0,0,4,0); ST(4,0,4,3); ST(4,3,0,3); ST(4,3,4,7);
            break;
        case 'S':
            ST(4,0,0,0); ST(0,0,0,3); ST(0,3,4,3); ST(4,3,4,7); ST(4,7,0,7);
            break;
        case 'T':
            ST(0,0,4,0); ST(2,0,2,7);
            break;
        case 'U':
            ST(0,0,0,7); ST(0,7,4,7); ST(4,7,4,0);
            break;
        case 'V':
            ST(0,0,0,7); ST(0,7,4,7); ST(4,7,4,0);
            break;
        case 'W':
            ST(0,0,0,7); ST(0,7,4,7); ST(4,7,4,0); ST(2,4,2,7);
            break;
        case 'X':
            ST(0,0,4,0); ST(0,3,4,3); ST(0,7,4,7); ST(2,0,2,7);
            break;
        case 'Y':
            ST(0,0,4,0); ST(2,0,2,7);
            break;
        case 'Z':
            ST(0,0,4,0); ST(4,0,4,3); ST(4,3,0,3); ST(0,3,0,7); ST(0,7,4,7);
            break;
        case '+':
            ST(2,1,2,6); ST(0,3,4,3);
            break;
        case '-':
            ST(0,3,4,3);
            break;
        case '=':
            ST(0,2,4,2); ST(0,5,4,5);
            break;
        case '.':
            ST(2,6,2,7);
            break;
        case '_':
            ST(0,7,4,7);
            break;
        case '(':
            ST(1,0,1,7); ST(1,0,3,0); ST(1,7,3,7);
            break;
        case ')':
            ST(3,0,3,7); ST(1,0,3,0); ST(1,7,3,7);
            break;
        default:
#undef ST
            return false;
    }

#undef ST
    writeFsmState = WRITE_FSM_MOVE_TO_START;
    return true;
}

static bool load_char_to_write_memory(char c) {
    uint8_t rows[GLYPH_HEIGHT];
    bool covered[GLYPH_HEIGHT][GLYPH_WIDTH] = {{false}};
    clear_write_memory();

#if WRITE_SIMPLE_LINE_FONT
    if (load_simple_line_char_to_write_memory(c)) return true;
    clear_write_memory();
#endif

    if (!glyph_rows(c, rows)) return false;
    if (to_upper_char(c) == ' ') {
        writeFsmState = WRITE_FSM_DONE;
        return true;
    }

    if (!save_column_strokes(rows, covered)) return false;

    for (uint8_t y = 0; y < GLYPH_HEIGHT; y++) {
        if ((y & 1u) == 0u) {
            if (!save_remaining_row_strokes_ltr(rows[y], y, covered[y])) return false;
        } else {
            if (!save_remaining_row_strokes_rtl(rows[y], y, covered[y])) return false;
        }
    }

    if (writeIndex == 0u && !save_original_row_strokes(rows)) return false;

    writeFsmState = WRITE_FSM_MOVE_TO_START;
    return true;
}

static bool run_write_fsm(void) {
    static int8_t currentX = 0;
    static int8_t currentY = 0;

    currentX = 0;
    currentY = 0;
    currentStroke = 0;
    write_servo_up();

    while (writeFsmState != WRITE_FSM_DONE && writeFsmState != WRITE_FSM_ERROR) {
        if (prioridad_stop_uart()) {
            write_servo_up();
            return false;
        }

        switch (writeFsmState) {
            case WRITE_FSM_MOVE_TO_START:
                if (currentStroke >= writeIndex) {
                    writeFsmState = WRITE_FSM_DONE;
                    break;
                }

                {
                    char buf[17];
                    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)(currentStroke + 1u), (unsigned)writeIndex);
                    kl_status("Moviendo inicio", buf);
                }

                if (!navigate_to((int32_t)writeMemory[currentStroke].startX - currentX,
                                 (int32_t)writeMemory[currentStroke].startY - currentY)) {
                    write_servo_up();
                    return false;
                }

                currentX = writeMemory[currentStroke].startX;
                currentY = writeMemory[currentStroke].startY;
                writeFsmState = WRITE_FSM_PEN_DOWN;
                break;

            case WRITE_FSM_PEN_DOWN:
                kl_status("Plumon abajo", "Escribiendo");
                write_servo_down();
                if (delay_ms_prioridad_write_down(WRITE_STROKE_SETTLE_MS)) {
                    write_servo_up();
                    return false;
                }
                writeFsmState = WRITE_FSM_TRACE;
                break;

            case WRITE_FSM_TRACE:
                {
                    char buf[17];
                    snprintf(buf, sizeof(buf), "%u/%u", (unsigned)(currentStroke + 1u), (unsigned)writeIndex);
                    kl_status("Trazando", buf);
                }

                if (!trace_to((int32_t)writeMemory[currentStroke].endX - currentX,
                              (int32_t)writeMemory[currentStroke].endY - currentY)) {
                    write_servo_up();
                    return false;
                }

                currentX = writeMemory[currentStroke].endX;
                currentY = writeMemory[currentStroke].endY;
                writeFsmState = WRITE_FSM_PEN_UP;
                break;

            case WRITE_FSM_PEN_UP:
                kl_status("Plumon arriba", "Siguiente");
                write_servo_up();
                if (delay_ms_prioridad(WRITE_STROKE_GAP_MS)) {
                    write_servo_up();
                    return false;
                }
                currentStroke++;
                writeFsmState = WRITE_FSM_MOVE_TO_START;
                break;

            default:
                writeFsmState = WRITE_FSM_ERROR;
                break;
        }
    }

    write_servo_up();

    if (writeFsmState == WRITE_FSM_ERROR) return false;
    if (!navigate_to((int32_t)CHAR_ADVANCE_UNITS - currentX, -currentY)) return false;
    return true;
}

// =======================================================
// LOGICA ROBOT
// =======================================================
typedef enum {
    MODO_AUTOMATICO = 0,
    MODO_MANUAL = 1
} ModoRobot;

ModoRobot modoActual = MODO_AUTOMATICO;
bool modoSimbolos = false;

char manualBuffer[MANUAL_BUFFER_SIZE];
uint8_t manualLen = 0;

char uartTextBuffer[UART_TEXT_BUFFER_SIZE];
uint8_t uartTextLen = 0;
char uartLineBuffer[UART_LINE_BUFFER_SIZE];
uint8_t uartLineLen = 0;

void reset_uart_text_buffer(void) {
    uartTextLen = 0;
    uartTextBuffer[0] = '\0';
}

void reset_uart_line_buffer(void) {
    uartLineLen = 0;
    uartLineBuffer[0] = '\0';
}

void mostrar_modo_actual(void) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    if (modoActual == MODO_AUTOMATICO) {
        lcd_print("MODO AUTO");
        lcd_set_cursor(1, 0);
        lcd_print("Esperando ESP");
        kl_status("MODO AUTO", "Esperando ESP");
    } else {
        lcd_print("MODO MANUAL");
        lcd_set_cursor(1, 0);
        lcd_print("# ok D simb");
        kl_status("MODO MANUAL", "Keypad activo");
    }
}

void mostrar_manual_buffer(void) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print(modoSimbolos ? "Simb texto:" : "Manual texto:");
    lcd_set_cursor(1, 0);
    if (manualLen <= 16) lcd_print_padded(manualBuffer);
    else                 lcd_print_padded(&manualBuffer[manualLen - 16]);
}

static bool escribir_letra_robot(char letra) {
    char textoLetra[2] = {letra, '\0'};

    if (stopRequested) return false;

    kl_status("Escribiendo letra", textoLetra);
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Escribiendo:");
    lcd_set_cursor(1, 0);
    lcd_data((uint8_t)letra);

    if (!load_char_to_write_memory(letra)) {
        kl_status("Letra no lista", textoLetra);
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Letra no lista");
        lcd_set_cursor(1, 0);
        lcd_print_padded(textoLetra);
        delay_ms(500);
        if (!navigate_to(CHAR_ADVANCE_UNITS, 0)) return false;
        return !stopRequested;
    }

    if (!run_write_fsm()) {
        if (stopRequested) return false;

        kl_status("Write detenido", textoLetra);
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Write detenido");
        lcd_set_cursor(1, 0);
        lcd_print_padded(textoLetra);
        delay_ms(500);
        return false;
    }

    return true;
}

static bool escribir_texto_robot(const char *texto) {
    kl_status("Escritura real", "Iniciando");
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Escritura real");
    lcd_set_cursor(1, 0);
    lcd_print("Iniciando...");
    delay_ms(600);

    for (uint8_t i = 0; texto[i] != '\0'; i++) {
        if (stopRequested) return false;
        if (!escribir_letra_robot(texto[i])) return false;
    }

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Texto listo");
    kl_status("Texto listo", texto);
    lcd_set_cursor(1, 0);
    if (strlen(texto) <= 16) lcd_print_padded(texto);
    else                     lcd_print_padded(&texto[strlen(texto) - 16]);
    delay_ms(800);
    return !stopRequested;
}

bool escribir_texto_automatico(const char *textoEntrada) {
    char textoFinal[FINAL_TEXT_BUFFER_SIZE];
    if (textoEntrada[0] == '\0') return true;

    if (modoActual == MODO_MANUAL) {
        uart0_print("BUSY_MANUAL\n");
        return false;
    }

    procesar_texto_antes_de_escribir(textoEntrada, textoFinal, sizeof(textoFinal));

    kl_status("Recibido ESP", textoEntrada);
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Recibido ESP:");
    lcd_set_cursor(1, 0);
    lcd_print_padded(textoEntrada);
    delay_ms(600);

    uart0_print("#INPUT:");
    uart0_print(textoEntrada);
    uart0_print("\n");

    uart0_print("#WRITING:");
    uart0_print(textoFinal);
    uart0_print("\n");

    if (!escribir_texto_robot(textoFinal)) {
        motors_stop();
        arm_stop();
        write_servo_up();
        kl_status("Escritura cancel", "STOP");
        mostrar_modo_actual();
        return false;
    }

    uart0_print("#DONE\n");
    kl_status("Escritura lista", "Esperando ESP");

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Esperando ESP");
    return true;
}

char traducir_tecla_manual(char key) {
    if (!modoSimbolos) {
        if (key == 'A') return ' ';
        return key;
    }
    switch (key) {
        case '1': return '+';  case '2': return '-';
        case '3': return '*';  case '4': return '/';
        case '5': return '=';  case '6': return '.';
        case '7': return '(';  case '8': return ')';
        case '9': return '^';  case '0': return '_';
        case 'A': return ' ';  default:  return '\0';
    }
}

void escribir_texto_manual(void) {
    char textoFinal[FINAL_TEXT_BUFFER_SIZE];
    stopRequested = false;

    if (manualLen == 0) {
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Texto vacio");
        delay_ms(700);
        mostrar_modo_actual();
        return;
    }

    procesar_texto_antes_de_escribir(manualBuffer, textoFinal, sizeof(textoFinal));

    uart0_print("#MANUAL:");
    uart0_print(manualBuffer);
    uart0_print("\n");
    kl_status("Manual recibido", manualBuffer);

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Confirmado");
    lcd_set_cursor(1, 0);
    lcd_print("Muevete...");
    delay_ms(MANUAL_START_DELAY_MS);

    uart0_print("#WRITING:");
    uart0_print(textoFinal);
    uart0_print("\n");

    if (!escribir_texto_robot(textoFinal)) {
        motors_stop();
        arm_stop();
        write_servo_up();
        kl_status("Manual cancelado", "STOP");
        mostrar_modo_actual();
        return;
    }

    uart0_print("#DONE\n");
    kl_status("Manual listo", "OK");

    manualLen = 0;
    manualBuffer[0] = '\0';

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Manual listo");
    delay_ms(900);
    mostrar_modo_actual();
}

void cambiar_modo(void) {
    if (modoActual == MODO_AUTOMATICO) {
        modoActual = MODO_MANUAL;
        modoSimbolos = false;
        manualLen = 0;
        manualBuffer[0] = '\0';
        motors_stop();
        arm_stop();
        write_servo_up();
        uart0_print("#MODE:MANUAL\n");
    } else {
        modoActual = MODO_AUTOMATICO;
        modoSimbolos = false;
        uart0_print("#MODE:AUTO\n");
    }
    mostrar_modo_actual();
}

void cambiar_modo_simbolos(void) {
    modoSimbolos = !modoSimbolos;
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print(modoSimbolos ? "Modo simbolos" : "Modo normal");
    lcd_set_cursor(1, 0);
    lcd_print(modoSimbolos ? "1+ 2- 3* 4/" : "Nums activos");
    delay_ms(900);

    if (modoActual == MODO_MANUAL && manualLen > 0) mostrar_manual_buffer();
    else mostrar_modo_actual();
}

void procesar_tecla(char key) {
    char caracter;
    if (key == 0) return;

    if (key == '*') { cambiar_modo(); return; }
    if (key == 'D') { cambiar_modo_simbolos(); return; }
    if (modoActual == MODO_AUTOMATICO) return;
    if (key == '#') { escribir_texto_manual(); return; }

    if (key == 'B') {
        if (manualLen > 0) { manualLen--; manualBuffer[manualLen] = '\0'; mostrar_manual_buffer(); }
        return;
    }
    if (key == 'C') {
        manualLen = 0; manualBuffer[0] = '\0'; mostrar_manual_buffer();
        return;
    }

    caracter = traducir_tecla_manual(key);
    if (caracter == '\0') return;

    if (manualLen < MANUAL_BUFFER_SIZE - 1) {
        manualBuffer[manualLen++] = caracter;
        manualBuffer[manualLen] = '\0';
        mostrar_manual_buffer();
    } else {
        lcd_clear(); lcd_set_cursor(0, 0); lcd_print("Buffer lleno");
        delay_ms(600); mostrar_manual_buffer();
    }
}

// =======================================================
// COMANDOS UART - MOVIMIENTO Y BORRADO
// =======================================================
void procesar_borrado_punto_cmd(const char *datos) {
    const char *p = datos;
    int32_t x_mm = parse_mm_value(&p);
    if (*p == ',') p++;
    int32_t y_mm = parse_mm_value(&p);
    if (*p == ',') p++;
    int32_t r_mm = parse_mm_value(&p);

    char linea[17];
    snprintf(linea, sizeof(linea), "(%ld,%ld)r%ld", (long)x_mm, (long)y_mm, (long)r_mm);

    kl_status("Moviendo punto", linea);
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Moviendo punto:");
    lcd_set_cursor(1, 0);
    lcd_print(linea);

    // Navegar a la posicion aproximada
    if (!navigate_to(x_mm, y_mm)) {
        mostrar_modo_actual();
        return;
    }

    // Extender brazo y hacer un pequeno barrido local
    kl_status("Barrido local", linea);
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Barrido local:");
    lcd_set_cursor(1, 0);
    lcd_print(linea);

    arm_extend();
    kl_status("Borrador abajo", linea);
    if (delay_ms_prioridad(ARM_ENGAGE_MS)) {
        mostrar_modo_actual();
        return;
    }

    turn_right();
    kl_status("Barrido derecha", linea);
    if (delay_ms_prioridad(r_mm * MS_PER_UNIT_H)) {
        mostrar_modo_actual();
        return;
    }
    motors_stop();
    if (delay_ms_prioridad(80)) {
        mostrar_modo_actual();
        return;
    }
    turn_left();
    kl_status("Barrido izquierda", linea);
    if (delay_ms_prioridad(r_mm * MS_PER_UNIT_H * 2u)) {
        mostrar_modo_actual();
        return;
    }
    motors_stop();
    if (delay_ms_prioridad(80)) {
        mostrar_modo_actual();
        return;
    }
    turn_right();
    kl_status("Centrando punto", linea);
    if (delay_ms_prioridad(r_mm * MS_PER_UNIT_H)) {
        mostrar_modo_actual();
        return;
    }
    motors_stop();

    arm_retract();
    kl_status("Borrador arriba", linea);
    if (delay_ms_prioridad(ARM_ENGAGE_MS)) {
        mostrar_modo_actual();
        return;
    }

    // Volver a la posicion de origen
    kl_status("Regresando", linea);
    if (!navigate_to(-x_mm, -y_mm)) {
        mostrar_modo_actual();
        return;
    }

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Punto borrado");
    kl_status("Punto borrado", linea);
    lcd_set_cursor(1, 0);
    lcd_print(linea);
    delay_ms(500);

    uart0_print("OK\n");
    mostrar_modo_actual();
}

void procesar_borrado(void) {
    kl_status("Comando", "ERASE");
    if (!erase_board_sweep()) {
        mostrar_modo_actual();
        return;
    }
    uart0_print("OK\n");
    mostrar_modo_actual();
}

// =======================================================
// COMANDOS DE MOVIMIENTO DESDE PAGINA
// =======================================================
// Estos comandos se escriben en la pagina como texto:
// (w) = avanzar recto
// (a) = izquierda
// (i) = derecha
// (d) = atras / bajar
// (x) = detener

static bool procesar_comando_pagina_movimiento(const char *texto) {
    if (strcmp(texto, "(w)") == 0 || strcmp(texto, "(W)") == 0) {
        move_forward();
        kl_status("Pagina cmd", "Avanzar recto");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Pagina: (w)");
        lcd_set_cursor(1, 0);
        lcd_print("Recto");
        uart0_print("OK_MOVE_W\n");
        return true;
    }

    if (strcmp(texto, "(a)") == 0 || strcmp(texto, "(A)") == 0) {
        turn_left();
        kl_status("Pagina cmd", "Izquierda");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Pagina: (a)");
        lcd_set_cursor(1, 0);
        lcd_print("Izquierda");
        uart0_print("OK_MOVE_A\n");
        return true;
    }

    if (strcmp(texto, "(i)") == 0 || strcmp(texto, "(I)") == 0) {
        turn_right();
        kl_status("Pagina cmd", "Derecha");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Pagina: (i)");
        lcd_set_cursor(1, 0);
        lcd_print("Derecha");
        uart0_print("OK_MOVE_I\n");
        return true;
    }

    if (strcmp(texto, "(d)") == 0 || strcmp(texto, "(D)") == 0) {
        move_backward();
        kl_status("Pagina cmd", "Atras");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Pagina: (d)");
        lcd_set_cursor(1, 0);
        lcd_print("Atras");
        uart0_print("OK_MOVE_D\n");
        return true;
    }

    if (strcmp(texto, "(x)") == 0 || strcmp(texto, "(X)") == 0) {
        stopRequested = true;
        motors_stop();
        arm_stop();
        write_servo_up();
        kl_status("Pagina cmd", "STOP");
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("Pagina: (x)");
        lcd_set_cursor(1, 0);
        lcd_print("STOP");
        uart0_print("OK_STOP\n");
        return true;
    }

    return false;
}

void procesar_fin_texto(void) {
    uartTextBuffer[uartTextLen] = '\0';
    stopRequested = false;

    // Primero revisa si el texto recibido desde la pagina es comando de movimiento
    if (procesar_comando_pagina_movimiento(uartTextBuffer)) {
        reset_uart_text_buffer();
        return;
    }

    if (uartTextLen == 2 && uartTextBuffer[0] == '@' && uartTextBuffer[1] == 'B') {
        reset_uart_text_buffer();
        procesar_borrado();
        return;
    }

    if (uartTextLen >= 2 && uartTextBuffer[0] == 'E') {
        procesar_borrado_punto_cmd(uartTextBuffer + 1);
        reset_uart_text_buffer();
        return;
    }

    bool ok = escribir_texto_automatico(uartTextBuffer);
    reset_uart_text_buffer();
    if (ok && !stopRequested) uart0_print("OK\n");
}

void procesar_linea_uart(const char *linea) {
    if (modoActual == MODO_MANUAL) {
        uart0_print("BUSY_MANUAL\n");
        return;
    }

    kl_status("Cmd UART", linea);

    // Comandos desde pagina como texto: (w), (a), (i), (d), (x)
    if (procesar_comando_pagina_movimiento(linea)) {
        return;
    }

    if (strcmp(linea, "X") != 0 && strcmp(linea, "M0") != 0 && strcmp(linea, "STOP") != 0) {
        stopRequested = false;
    }

    if (strncmp(linea, "WRITE:", 6) == 0) {
        bool ok = escribir_texto_automatico(linea + 6);
        if (ok && !stopRequested) uart0_print("OK\n");
        return;
    }

    if (strcmp(linea, "ERASE") == 0) {
        procesar_borrado();
        return;
    }

    if (strncmp(linea, "ERASE_POINT:", 12) == 0) {
        procesar_borrado_punto_cmd(linea + 12);
        return;
    }

    // --- Comandos de movimiento ---
    if (strcmp(linea, "W") == 0) {
        move_forward();
        kl_status("Motor W", "Adelante");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Motor: W");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "S") == 0) {
        move_backward();
        kl_status("Motor S", "Atras");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Motor: S");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "A") == 0) {
        turn_left();
        kl_status("Motor A", "Izquierda");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Motor: A");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "D") == 0) {
        turn_right();
        kl_status("Motor D", "Derecha");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Motor: D");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "X") == 0 || strcmp(linea, "STOP") == 0) {
        stopRequested = true;
        reset_uart_line_buffer();
        reset_uart_text_buffer();
        motors_stop();
        arm_stop();
        write_servo_up();
        kl_status("Motor X", "Parado");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Motor: X");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }

    // --- Comandos del brazo eraser ---
    if (strcmp(linea, "FU") == 0) {
        arm_extend();
        kl_status("Borrador", "Abajo");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Brazo: FU");
        lcd_set_cursor(1,0); lcd_print_padded("Extendiendo");
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "FD") == 0) {
        arm_retract();
        kl_status("Borrador", "Arriba");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Brazo: FD");
        lcd_set_cursor(1,0); lcd_print_padded("Retrayendo");
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "FL") == 0) {
        fine_left();
        kl_status("Motor fino", "Izquierda");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Motor: FL");
        lcd_set_cursor(1,0); lcd_print_padded("Fino izq");
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "FR") == 0) {
        fine_right();
        kl_status("Motor fino", "Derecha");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Motor: FR");
        lcd_set_cursor(1,0); lcd_print_padded("Fino der");
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "F0") == 0) {
        arm_stop();
        kl_status("Borrador", "Parado");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Brazo: F0");
        lcd_set_cursor(1,0); lcd_print_padded("Parado");
        uart0_print("OK\n");
        return;
    }


    // --- Prueba individual de motores ---
    if (strcmp(linea, "M1F") == 0) {
        motor_set(MOTOR_1, DIR_FORWARD, DRIVE_SPEED);
        kl_status("Motor 1", "Adelante");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("M1 adelante");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "M1B") == 0) {
        motor_set(MOTOR_1, DIR_BACKWARD, DRIVE_SPEED);
        kl_status("Motor 1", "Atras");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("M1 atras");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "M2F") == 0) {
        motor_set(MOTOR_2, DIR_FORWARD, DRIVE_SPEED);
        kl_status("Motor 2", "Adelante");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("M2 adelante");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "M2B") == 0) {
        motor_set(MOTOR_2, DIR_BACKWARD, DRIVE_SPEED);
        kl_status("Motor 2", "Atras");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("M2 atras");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "M3F") == 0) {
        motor_set(MOTOR_3, DIR_FORWARD, DRIVE_SPEED);
        kl_status("Motor 3", "Adelante");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("M3 adelante");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "M3B") == 0) {
        motor_set(MOTOR_3, DIR_BACKWARD, DRIVE_SPEED);
        kl_status("Motor 3", "Atras");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("M3 atras");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "M4F") == 0) {
        motor_set(MOTOR_4, DIR_FORWARD, DRIVE_SPEED);
        kl_status("Motor 4", "Adelante");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("M4 adelante");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "M4B") == 0) {
        motor_set(MOTOR_4, DIR_BACKWARD, DRIVE_SPEED);
        kl_status("Motor 4", "Atras");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("M4 atras");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }
    if (strcmp(linea, "M0") == 0) {
        stopRequested = true;
        reset_uart_line_buffer();
        reset_uart_text_buffer();
        motors_stop();
        arm_stop();
        write_servo_up();
        kl_status("Motores", "STOP");
        lcd_clear(); lcd_set_cursor(0,0); lcd_print("Motores STOP");
        lcd_set_cursor(1,0); lcd_print_rx_tail(linea);
        uart0_print("OK\n");
        return;
    }

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("Cmd desconocido");
    lcd_set_cursor(1, 0);
    lcd_print_padded(linea);
    uart0_print("ERROR_CMD\n");
}

static bool uart_es_cmd_corto(const char *linea) {
    return strcmp(linea, "W") == 0 ||
           strcmp(linea, "S") == 0 ||
           strcmp(linea, "A") == 0 ||
           strcmp(linea, "D") == 0 ||
           strcmp(linea, "X") == 0 ||
           strcmp(linea, "M0") == 0 ||
           strcmp(linea, "FU") == 0 ||
           strcmp(linea, "FD") == 0 ||
           strcmp(linea, "FL") == 0 ||
           strcmp(linea, "FR") == 0 ||
           strcmp(linea, "F0") == 0 ||
           strcmp(linea, "M1F") == 0 ||
           strcmp(linea, "M1B") == 0 ||
           strcmp(linea, "M2F") == 0 ||
           strcmp(linea, "M2B") == 0 ||
           strcmp(linea, "M3F") == 0 ||
           strcmp(linea, "M3B") == 0 ||
           strcmp(linea, "M4F") == 0 ||
           strcmp(linea, "M4B") == 0;
}

static bool uart_es_borrado_punto(const char *linea) {
    return strncmp(linea, "ERASE_POINT:", 12) == 0 && strlen(linea) > 12u;
}

static bool uart_parece_texto_sin_fin(const char *linea) {
    if (strncmp(linea, "WRITE:", 6) == 0) return true;

    for (uint8_t i = 0; linea[i] != '\0'; i++) {
        char c = linea[i];

        if ((c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') ||
            c == ' ' || c == '+' || c == '-' || c == '*' ||
            c == '/' || c == '=' || c == '.') {
            continue;
        }

        return false;
    }

    return strlen(linea) >= 2u;
}

static void uart_normalizar_mayusculas(char *linea) {
    for (uint8_t i = 0; linea[i] != '\0'; i++) {
        if (linea[i] >= 'a' && linea[i] <= 'z') {
            linea[i] = (char)(linea[i] - ('a' - 'A'));
        }
    }
}

void procesar_uart_automatico(void) {
    static uint32_t ultimoRxMs = 0;

    if (uartLineLen > 0 && (msTicks - ultimoRxMs) > UART_RX_TIMEOUT_MS) {
        char lineaTimeout[UART_LINE_BUFFER_SIZE];

        copiar_seguro(lineaTimeout, uartLineBuffer, UART_LINE_BUFFER_SIZE);

        if (uart_parece_texto_sin_fin(uartLineBuffer)) {
            bool ok;

            kl_status("UART texto sin fin", uartLineBuffer);
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Texto sin fin");
            lcd_set_cursor(1, 0);
            lcd_print_rx_tail(uartLineBuffer);

            if (strncmp(uartLineBuffer, "WRITE:", 6) == 0) {
                procesar_linea_uart(uartLineBuffer);
            } else {
                ok = escribir_texto_automatico(uartLineBuffer);
                if (ok && !stopRequested) uart0_print("OK\n");
            }

            reset_uart_line_buffer();
            reset_uart_text_buffer();
            return;
        }

        uart_normalizar_mayusculas(lineaTimeout);

        if (uart_es_borrado_punto(lineaTimeout)) {
            kl_status("UART erase sin fin", lineaTimeout);
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Erase sin fin OK");
            lcd_set_cursor(1, 0);
            lcd_print_rx_tail(lineaTimeout);
            procesar_linea_uart(lineaTimeout);
            reset_uart_line_buffer();
            reset_uart_text_buffer();
            return;
        }

        if (uart_es_cmd_corto(lineaTimeout)) {
            kl_status("UART sin fin OK", lineaTimeout);
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("UART sin fin OK");
            lcd_set_cursor(1, 0);
            lcd_print_padded(lineaTimeout);
            procesar_linea_uart(lineaTimeout);
            reset_uart_line_buffer();
            reset_uart_text_buffer();
            return;
        }

        kl_status("UART incompleto", uartLineBuffer);
        lcd_clear();
        lcd_set_cursor(0, 0);
        lcd_print("UART incompleto");
        lcd_set_cursor(1, 0);
        if (uartLineLen <= 16) lcd_print_padded(uartLineBuffer);
        else                   lcd_print_padded(&uartLineBuffer[uartLineLen - 16]);
        reset_uart_line_buffer();
        reset_uart_text_buffer();
        uart0_print("ERROR_UART_TIMEOUT\n");
        return;
    }

    while (uart0_available()) {
        char c = uart0_getchar_blocking();
        ultimoRxMs = msTicks;

        if (c == '\r') continue;

        if (modoActual == MODO_MANUAL) {
            uart0_print("BUSY_MANUAL\n");
            return;
        }

        if (c == '\n') {
            uartLineBuffer[uartLineLen] = '\0';
            kl_status("Recibido linea", uartLineBuffer);
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Recibido linea");
            lcd_set_cursor(1, 0);
            lcd_print_rx_tail(uartLineBuffer);
            delay_ms(UART_RX_DISPLAY_MS);
            if (uartLineLen > 0) procesar_linea_uart(uartLineBuffer);
            reset_uart_line_buffer();
            reset_uart_text_buffer();
            return;
        }

        if (c == '#') {
            copiar_seguro(uartTextBuffer, uartLineBuffer, UART_TEXT_BUFFER_SIZE);
            uartTextLen = (uint8_t)strlen(uartTextBuffer);
            kl_status("Recibido texto", uartTextBuffer);
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Recibido texto");
            lcd_set_cursor(1, 0);
            lcd_print_rx_tail(uartTextBuffer);
            delay_ms(UART_RX_DISPLAY_MS);
            procesar_fin_texto();
            reset_uart_line_buffer();
            return;
        }

        if (uartLineLen < UART_LINE_BUFFER_SIZE - 1) {
            uartLineBuffer[uartLineLen++] = c;
            uartLineBuffer[uartLineLen]   = '\0';
        } else {
            reset_uart_line_buffer();
            uart0_print("ERROR_BUFFER\n");
            return;
        }
    }

    // No se actualiza el LCD por cada caracter: eso hacia lento el flujo UART.
}

// =======================================================
// MAIN
// =======================================================
int main(void) {
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u);

    lcd_init();
    uart0_init();
    keypad_init();
    tpm_motor_init();
    servos_init();

    manualBuffer[0]    = '\0';
    uartTextBuffer[0]  = '\0';
    uartLineBuffer[0]  = '\0';

    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("KL25Z MATT");
    lcd_set_cursor(1, 0);
    lcd_print("Motores listos");
    kl_status("KL25Z MATT", "Motores listos");
    delay_ms(1500);

    mostrar_modo_actual();

    while (1) {
        procesar_uart_automatico();

        char key = keypad_getkey();
        if (key != 0) procesar_tecla(key);

        delay_ms(1);
    }
}
