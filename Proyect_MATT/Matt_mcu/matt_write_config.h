    #ifndef MATT_WRITE_CONFIG_H
    #define MATT_WRITE_CONFIG_H

    #include <stdint.h>

    // =======================================================
    // CALIBRACION DE MOVIMIENTO PARA ESCRITURA/BORRADO
    // =======================================================
    // Limite motores: 0 a 70 (% PWM).
    // Si pones mas de 70, motor_set() lo recorta a 70.
    #define MOTOR_SPEED_MIN         0u
    #define MOTOR_SPEED_MAX         70u

    #define DRIVE_SPEED             MOTOR_SPEED_MAX
    #define FINE_SPEED              60u

    // Velocidad mientras el plumon esta abajo (trazo). Mas lento = linea
    // continua en vez de punteada. Si el robot no se mueve a este valor por
    // friccion en el tablero vertical, subelo (p.ej. 60-70).
    #define TRACE_SPEED             55u

    // =======================================================
    // MAPA DE MOVIMIENTO PARA 4 LLANTAS A 45 GRADOS
    // =======================================================
    // Convencion:  1 = DIR_FORWARD, -1 = DIR_BACKWARD, 0 = apagado.
    // Si un movimiento sale al lado contrario, cambia el signo de ese movimiento.
    // Si SIEMPRE una llanta empuja al reves, cambia MOTOR_X_SIGN.
    // Segun la prueba con encoders, avanzar recto usa:
    // M1 FORWARD, M2 BACKWARD, M3 FORWARD, M4 BACKWARD.
    #define MOTOR_1_SIGN            1
    #define MOTOR_2_SIGN           -1
    #define MOTOR_3_SIGN            1
    #define MOTOR_4_SIGN           -1

    // Trim por motor. Baja el motor que jala de mas, sube el que se queda corto.
    #define MOTOR_1_TRIM            60u
    #define MOTOR_2_TRIM            60u
    #define MOTOR_3_TRIM            100u
    #define MOTOR_4_TRIM            100u

    // NOTA: el orden fisico real puede diferir. En este robot:
    // M1 y M2 = mismo lado del robot
    // M3 y M4 = lado opuesto del robot
    // Patron base tipo X/omni 45 grados.
    #define MOVE_FWD_M1             1
    #define MOVE_FWD_M2             1
    #define MOVE_FWD_M3             1
    #define MOVE_FWD_M4             1

    #define MOVE_BACK_M1           -1
    #define MOVE_BACK_M2           -1
    #define MOVE_BACK_M3           -1
    #define MOVE_BACK_M4           -1

    #define MOVE_LEFT_M1           -1
    #define MOVE_LEFT_M2            1
    #define MOVE_LEFT_M3            1
    #define MOVE_LEFT_M4           -1

    #define MOVE_RIGHT_M1           1
    #define MOVE_RIGHT_M2          -1
    #define MOVE_RIGHT_M3          -1
    #define MOVE_RIGHT_M4           1

    #define SWEEP_ROW_MS            7000u
    #define SWEEP_STEP_MS           1000u
    #define SWEEP_ROWS              5u
    #define ARM_ENGAGE_MS           700u

    #define MS_PER_UNIT_H           28u
    #define MS_PER_UNIT_V           28u

    // El trazo va mas lento que la navegacion, asi que se escala el tiempo por
    // unidad para recorrer la MISMA distancia fisica y no deformar la letra.
    #define TRACE_MS_PER_UNIT_H     (MS_PER_UNIT_H * DRIVE_SPEED / TRACE_SPEED)
    #define TRACE_MS_PER_UNIT_V     (MS_PER_UNIT_V * DRIVE_SPEED / TRACE_SPEED)

    #define WRITE_SCALE_UNITS       2
    #define CHAR_ADVANCE_UNITS      14
    #define WRITE_SIMPLE_LINE_FONT  1
    // Tiempo que el plumon asienta antes de moverse: deja fluir tinta para que
    // el inicio del trazo no quede como un punto suelto.
    #define WRITE_STROKE_SETTLE_MS  120u
    #define WRITE_STROKE_GAP_MS     40u

    // =======================================================
    // SERVOS DE ESCRITURA Y BORRADO
    // =======================================================
    #define SERVO_WRITE_PORT_CFG    PORTD
    #define SERVO_WRITE_GPIO        GPIOD
    #define SERVO_WRITE_PIN         5u

    #define SERVO_ERASE_PORT_CFG    PORTE
    #define SERVO_ERASE_GPIO        GPIOE
    #define SERVO_ERASE_PIN         23u

    #define SERVO_UP_DEG            0u
    #define SERVO_DOWN_DEG          70u
    #define SERVO_ERASE_UP_DEG      70u
    #define SERVO_ERASE_DOWN_DEG    0u
    #define SERVO_HOLD_MS           450u

    // =======================================================
    // MEMORIA Y TAMANO DE LETRA
    // =======================================================
    #define WRITE_MAX_STROKES       40u
    #define GLYPH_WIDTH             5u
    #define GLYPH_HEIGHT            7u

    typedef struct {
        char symbol;
        uint8_t rows[GLYPH_HEIGHT];
    } glyph_definition_t;

    static const uint8_t MATT_GLYPH_BLANK[GLYPH_HEIGHT] = {
        0, 0, 0, 0, 0, 0, 0
    };

    // Cada fila usa 5 bits. Ejemplo: 0x1F = 11111, 0x11 = 10001.
    static const glyph_definition_t MATT_GLYPHS[] = {
        {'0', {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}},
        {'1', {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}},
        {'2', {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F}},
        {'3', {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E}},
        {'4', {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}},
        {'5', {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E}},
        {'6', {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E}},
        {'7', {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}},
        {'8', {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}},
        {'9', {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E}},

        {'A', {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}},
        {'B', {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}},
        {'C', {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}},
        {'D', {0x1E,0x11,0x11,0x11,0x11,0x11,0x1E}},
        {'E', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}},
        {'F', {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}},
        {'G', {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E}},
        {'H', {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}},
        {'I', {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}},
        {'J', {0x07,0x02,0x02,0x02,0x12,0x12,0x0C}},
        {'K', {0x11,0x12,0x14,0x18,0x14,0x12,0x11}},
        {'L', {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}},
        {'M', {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}},
        {'N', {0x11,0x19,0x15,0x13,0x11,0x11,0x11}},
        {'O', {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}},
        {'P', {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}},
        {'Q', {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}},
        {'R', {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}},
        {'S', {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}},
        {'T', {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}},
        {'U', {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}},
        {'V', {0x11,0x11,0x11,0x11,0x0A,0x0A,0x04}},
        {'W', {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}},
        {'X', {0x11,0x0A,0x0A,0x04,0x0A,0x0A,0x11}},
        {'Y', {0x11,0x0A,0x0A,0x04,0x04,0x04,0x04}},
        {'Z', {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}},

        {'+', {0x00,0x04,0x04,0x1F,0x04,0x04,0x00}},
        {'-', {0x00,0x00,0x00,0x1F,0x00,0x00,0x00}},
        {'*', {0x00,0x11,0x0A,0x04,0x0A,0x11,0x00}},
        {'/', {0x01,0x02,0x02,0x04,0x08,0x08,0x10}},
        {'=', {0x00,0x00,0x1F,0x00,0x1F,0x00,0x00}},
        {'.', {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C}},
        {'_', {0x00,0x00,0x00,0x00,0x00,0x00,0x1F}},
        {'^', {0x04,0x0A,0x11,0x00,0x00,0x00,0x00}},
        {'(', {0x02,0x04,0x08,0x08,0x08,0x04,0x02}},
        {')', {0x08,0x04,0x02,0x02,0x02,0x04,0x08}},
    };

    #define MATT_GLYPH_COUNT (sizeof(MATT_GLYPHS) / sizeof(MATT_GLYPHS[0]))

    #endif
