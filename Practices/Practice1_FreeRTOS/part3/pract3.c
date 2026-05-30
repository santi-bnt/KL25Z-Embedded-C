/*
 *
 * Parte 3 usando:
 * - LDR real por ADC directo en registros
 * - DHT11 real por GPIO digital
 * - Boton real por GPIO con pull-down interno
 * - Queue para comunicar datos
 * - Salida por terminal serial
 *
 * Pines:
 * - LDR ADC    -> PTB1 / ADC0_SE9
 * - Boton      -> PTB0
 * - DHT11 DATA -> PTC2
 *
 * Boton:
 * PTB0 ---- boton ---- 3.3V
 *
 * Logica boton:
 * 0 = no presionado
 * 1 = presionado
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"

#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_port.h"

#include "MKL25Z4.h"

/* =========================
 * Pines
 * ========================= */

#define BUTTON_GPIO GPIOB
#define BUTTON_PORT PORTB
#define BUTTON_PIN  0U          /* PTB0 */

#define DHT11_GPIO  GPIOC
#define DHT11_PORT  PORTC
#define DHT11_PIN   2U          /* PTC2 */

#define LDR_PORT    PORTB
#define LDR_PIN     1U          /* PTB1 / ADC0_SE9 */

#define ADC_CH_LIGHT 9U         /* ADC0_SE9 */

/* =========================
 * Configuracion
 * ========================= */

#define ADC_RESOLUTION_VALUE 4096U
#define ADC_VREF_MV          3300U

#define LIGHT_THRESHOLD 1800U
#define TEMP_THRESHOLD 30U

/* =========================
 * Tipos de mensaje
 * ========================= */

typedef enum
{
    SENSOR_LIGHT,
    SENSOR_TEMP,
    SENSOR_HUMIDITY,
    SENSOR_BUTTON
} sensor_type_t;

typedef struct
{
    sensor_type_t type;
    uint16_t value;
} sensor_msg_t;

/* =========================
 * Queue global
 * ========================= */

QueueHandle_t sensorQueue;

/* =========================
 * Prototipos
 * ========================= */

static void vLightSensor(void *pvParameters);
static void vTempSensor(void *pvParameters);
static void vButtonTask(void *pvParameters);
static void vSerialTask(void *pvParameters);

static void ADC0_InitManual(void);
static uint16_t ADC0_ReadChannel(uint8_t channel);

static void DHT11_InitPin(void);
static void DHT11_SetOutput(void);
static void DHT11_SetInput(void);
static void DHT11_WriteLow(void);
static uint8_t DHT11_ReadPin(void);
static bool DHT11_WaitForLevel(uint8_t level, uint32_t timeout_us);
static bool DHT11_Read(uint8_t *temperature, uint8_t *humidity);

static void DelayUs(uint32_t us);

/* =========================
 * Main
 * ========================= */

int main(void)
{
    gpio_pin_config_t button_gpio_config =
    {
        kGPIO_DigitalInput,
        0,
    };

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    /* Habilitar clocks */
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);

    /* =========================
     * Boton PTB0 con pull-down
     * ========================= */

    PORT_SetPinMux(BUTTON_PORT, BUTTON_PIN, kPORT_MuxAsGpio);

    /*
     * Pull-down interno:
     * PE = 1 habilita pull
     * PS = 0 selecciona pull-down
     *
     * Conexion:
     * PTB0 ---- boton ---- 3.3V
     */
    BUTTON_PORT->PCR[BUTTON_PIN] |= PORT_PCR_PE_MASK;
    BUTTON_PORT->PCR[BUTTON_PIN] &= ~PORT_PCR_PS_MASK;

    GPIO_PinInit(BUTTON_GPIO, BUTTON_PIN, &button_gpio_config);

    /* =========================
     * LDR PTB1 / ADC0_SE9
     * ========================= */

    /*
     * Deshabilitar mux digital para usar PTB1 como ADC.
     */
    PORTB->PCR[LDR_PIN] = 0x00000000;

    /* =========================
     * DHT11 PTC2
     * ========================= */

    PORT_SetPinMux(DHT11_PORT, DHT11_PIN, kPORT_MuxAsGpio);

    /*
     * Pull-up interno para DHT11.
     * Aun asi, se recomienda resistencia fisica de 10k entre DATA y 3.3V.
     */
    DHT11_PORT->PCR[DHT11_PIN] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;

    DHT11_InitPin();

    /* ADC */
    ADC0_InitManual();

    /* Queue */
    sensorQueue = xQueueCreate(10, sizeof(sensor_msg_t));

    if(sensorQueue == NULL)
    {
        PRINTF("Error creating queue\r\n");
        while(1)
        {
        }
    }

    PRINTF("\r\nFreeRTOS KL25Z - LDR + DHT11 REAL + Button + Queue\r\n");
    PRINTF("LDR: PTB1 / ADC0_SE9\r\n");
    PRINTF("DHT11 DATA: PTC2\r\n");
    PRINTF("Boton: PTB0 pull-down\r\n");
    PRINTF("Boton: 0 = no presionado | 1 = presionado\r\n");
    PRINTF("Sistema iniciado\r\n\r\n");

    xTaskCreate(vLightSensor,
                "Light",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                2,
                NULL);

    xTaskCreate(vTempSensor,
                "DHT11",
                configMINIMAL_STACK_SIZE + 300,
                NULL,
                2,
                NULL);

    xTaskCreate(vButtonTask,
                "Button",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                1,
                NULL);

    xTaskCreate(vSerialTask,
                "Serial",
                configMINIMAL_STACK_SIZE + 250,
                NULL,
                3,
                NULL);

    vTaskStartScheduler();

    while(1)
    {
    }
}

/* =========================================================
 * ADC0 manual por registros
 * ========================================================= */

static void ADC0_InitManual(void)
{
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    /*
     * ADC0_CFG1:
     * ADIV = 1    -> divide clock entre 2
     * MODE = 01   -> 12 bits
     * ADICLK = 00 -> bus clock
     */
    ADC0->CFG1 = ADC_CFG1_ADIV(1) |
                 ADC_CFG1_MODE(1) |
                 ADC_CFG1_ADICLK(0);

    ADC0->SC2 = 0;
    ADC0->SC2 &= ~ADC_SC2_ADTRG_MASK;
}

static uint16_t ADC0_ReadChannel(uint8_t channel)
{
    ADC0->SC1[0] = ADC_SC1_ADCH(channel);

    while((ADC0->SC1[0] & ADC_SC1_COCO_MASK) == 0)
    {
    }

    return (uint16_t)(ADC0->R[0]);
}

/* =========================
 * Tarea LDR real
 * ========================= */

static void vLightSensor(void *pvParameters)
{
    sensor_msg_t msg;

    while(1)
    {
        uint16_t result;

        result = ADC0_ReadChannel(ADC_CH_LIGHT);

        msg.type = SENSOR_LIGHT;
        msg.value = result;

        xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* =========================
 * Tarea DHT11 real
 * ========================= */

static void vTempSensor(void *pvParameters)
{
    sensor_msg_t msgTemp;
    sensor_msg_t msgHumidity;

    uint8_t temperature = 0;
    uint8_t humidity = 0;

    while(1)
    {
        PRINTF("DHT11 idle pin: %u\r\n", DHT11_ReadPin());

        if(DHT11_Read(&temperature, &humidity))
        {
            msgTemp.type = SENSOR_TEMP;
            msgTemp.value = temperature;
            xQueueSend(sensorQueue, &msgTemp, pdMS_TO_TICKS(10));

            msgHumidity.type = SENSOR_HUMIDITY;
            msgHumidity.value = humidity;
            xQueueSend(sensorQueue, &msgHumidity, pdMS_TO_TICKS(10));

            PRINTF("DHT11 OK -> Temp: %u C | Humidity: %u %%\r\n",
                   temperature,
                   humidity);
        }
        else
        {
            PRINTF("DHT11 read error\r\n");
        }

        /*
         * DHT11 no debe leerse muy rapido.
         */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

/* =========================
 * Tarea boton real
 * ========================= */

static void vButtonTask(void *pvParameters)
{
    sensor_msg_t msg;

    uint32_t button_state = 0;
    uint32_t last_state = 0;

    while(1)
    {
        button_state = GPIO_ReadPinInput(BUTTON_GPIO, BUTTON_PIN);

        if(button_state != last_state)
        {
            if(button_state == 1)
            {
                PRINTF("Button pressed: 1\r\n");
            }
            else
            {
                PRINTF("Button released: 0\r\n");
            }

            last_state = button_state;
        }

        msg.type = SENSOR_BUTTON;
        msg.value = (uint16_t)button_state;

        xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));

        vTaskDelay(pdMS_TO_TICKS(80));
    }
}

/* =========================
 * Tarea serial
 * ========================= */

static void vSerialTask(void *pvParameters)
{
    sensor_msg_t msg;

    uint16_t light_raw = 0;
    uint16_t temperature = 0;
    uint16_t humidity = 0;
    uint16_t button = 0;

    TickType_t lastPrintTime = xTaskGetTickCount();

    while(1)
    {
        if(xQueueReceive(sensorQueue, &msg, pdMS_TO_TICKS(100)) == pdPASS)
        {
            switch(msg.type)
            {
                case SENSOR_LIGHT:
                    light_raw = msg.value;
                    break;

                case SENSOR_TEMP:
                    temperature = msg.value;
                    break;

                case SENSOR_HUMIDITY:
                    humidity = msg.value;
                    break;

                case SENSOR_BUTTON:
                    button = msg.value;
                    break;

                default:
                    break;
            }
        }

        if((xTaskGetTickCount() - lastPrintTime) >= pdMS_TO_TICKS(1500))
        {
            uint32_t light_voltage_mv;
            uint32_t light_percent;

            /*
             * Formula tipo slide aplicada al ADC de luz.
             * ADC 12 bits:
             * valor = result * escala / 4096
             */
            light_voltage_mv = ((uint32_t)light_raw * ADC_VREF_MV) / ADC_RESOLUTION_VALUE;
            light_percent = ((uint32_t)light_raw * 100U) / ADC_RESOLUTION_VALUE;

            PRINTF("Light raw: %u | Light voltage: %u mV | Light: %u %% | Temp DHT11: %u C | Humidity: %u %% | Button: %u\r\n",
                   light_raw,
                   light_voltage_mv,
                   light_percent,
                   temperature,
                   humidity,
                   button);

            if(light_raw < LIGHT_THRESHOLD)
            {
                PRINTF("Light status: DARK\r\n");
            }
            else
            {
                PRINTF("Light status: BRIGHT\r\n");
            }

            if(temperature > TEMP_THRESHOLD)
            {
                PRINTF("Temperature status: HIGH\r\n");
            }
            else
            {
                PRINTF("Temperature status: NORMAL\r\n");
            }

            if(button == 1)
            {
                PRINTF("Button status: PRESSED | Button value: 1\r\n");
            }
            else
            {
                PRINTF("Button status: NOT PRESSED | Button value: 0\r\n");
            }

            PRINTF("\r\n");

            lastPrintTime = xTaskGetTickCount();
        }
    }
}

/* =========================================================
 * Delay aproximado en microsegundos
 * ========================================================= */

static void DelayUs(uint32_t us)
{
    volatile uint32_t count;

    while(us--)
    {
        /*
         * Ajuste aproximado para KL25Z.
         * Si sale checksum error, prueba con 12, 15, 20 o 25.
         */
        count = 15;

        while(count--)
        {
            __NOP();
        }
    }
}

/* =========================================================
 * Funciones DHT11
 * ========================================================= */

static void DHT11_InitPin(void)
{
    DHT11_SetInput();
}

static void DHT11_SetOutput(void)
{
    gpio_pin_config_t config =
    {
        kGPIO_DigitalOutput,
        1,
    };

    GPIO_PinInit(DHT11_GPIO, DHT11_PIN, &config);

    DHT11_PORT->PCR[DHT11_PIN] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
}

static void DHT11_SetInput(void)
{
    gpio_pin_config_t config =
    {
        kGPIO_DigitalInput,
        0,
    };

    GPIO_PinInit(DHT11_GPIO, DHT11_PIN, &config);

    DHT11_PORT->PCR[DHT11_PIN] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
}

static void DHT11_WriteLow(void)
{
    GPIO_WritePinOutput(DHT11_GPIO, DHT11_PIN, 0);
}

static uint8_t DHT11_ReadPin(void)
{
    return (uint8_t)GPIO_ReadPinInput(DHT11_GPIO, DHT11_PIN);
}

static bool DHT11_WaitForLevel(uint8_t level, uint32_t timeout_us)
{
    while(timeout_us--)
    {
        if(DHT11_ReadPin() == level)
        {
            return true;
        }

        DelayUs(1);
    }

    return false;
}

static bool DHT11_Read(uint8_t *temperature, uint8_t *humidity)
{
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint8_t i;
    uint8_t j;

    taskENTER_CRITICAL();

    /*
     * Start signal:
     * MCU baja DATA por minimo 18 ms.
     */
    DHT11_SetOutput();
    DHT11_WriteLow();

    DelayUs(20000);

    /*
     * Liberar linea.
     */
    GPIO_WritePinOutput(DHT11_GPIO, DHT11_PIN, 1);
    DelayUs(30);

    DHT11_SetInput();

    /*
     * Respuesta esperada del DHT11:
     * LOW  ~80 us
     * HIGH ~80 us
     * LOW  ~50 us
     */
    if(!DHT11_WaitForLevel(0, 300))
    {
        taskEXIT_CRITICAL();
        PRINTF("DHT11 fail: no initial LOW\r\n");
        return false;
    }

    if(!DHT11_WaitForLevel(1, 300))
    {
        taskEXIT_CRITICAL();
        PRINTF("DHT11 fail: no initial HIGH\r\n");
        return false;
    }

    if(!DHT11_WaitForLevel(0, 300))
    {
        taskEXIT_CRITICAL();
        PRINTF("DHT11 fail: no data LOW\r\n");
        return false;
    }

    /*
     * Leer 40 bits.
     */
    for(i = 0; i < 5; i++)
    {
        for(j = 0; j < 8; j++)
        {
            uint32_t high_time = 0;

            if(!DHT11_WaitForLevel(1, 300))
            {
                taskEXIT_CRITICAL();
                PRINTF("DHT11 fail: bit no HIGH\r\n");
                return false;
            }

            while(DHT11_ReadPin() == 1)
            {
                high_time++;
                DelayUs(1);

                if(high_time > 300)
                {
                    taskEXIT_CRITICAL();
                    PRINTF("DHT11 fail: bit HIGH timeout\r\n");
                    return false;
                }
            }

            data[i] <<= 1;

            /*
             * HIGH corto = 0
             * HIGH largo = 1
             */
            if(high_time > 45)
            {
                data[i] |= 1;
            }
        }
    }

    taskEXIT_CRITICAL();

    /*
     * Checksum.
     */
    if(data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF))
    {
        PRINTF("DHT11 fail: checksum | data: %u %u %u %u %u\r\n",
               data[0],
               data[1],
               data[2],
               data[3],
               data[4]);

        return false;
    }

    /*
     * DHT11:
     * data[0] = humedad entera
     * data[1] = humedad decimal
     * data[2] = temperatura entera
     * data[3] = temperatura decimal
     */
    *humidity = data[0];
    *temperature = data[2];

    return true;
}