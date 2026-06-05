/*
 * main.c - FreeRTOS KL25Z
 * Parte 2: Tasks using Queues
 *
 * Pot 1 -> PTB1 / ADC0_SE9  (Light)
 * Pot 2 -> PTB2 / ADC0_SE12 (Temp)
 * Button -> PTA1
 *
 * Button with inverted logic:
 * Physical pin = 1 -> not pressed
 * Physical pin = 0 -> pressed
 *
 * Program logic:
 * Button = 0 -> not pressed
 * Button = 1 -> pressed
 *
 * Button connection:
 * PTA1 ---- button ---- GND
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
#include "fsl_adc16.h"

/* =========================
 * Message Types
 * ========================= */

typedef enum
{
    SENSOR_LIGHT,
    SENSOR_TEMP,
    SENSOR_BUTTON
} sensor_type_t;

typedef struct
{
    sensor_type_t type;
    uint16_t value;
} sensor_msg_t;

/* =========================
 * Queue
 * ========================= */

QueueHandle_t sensorQueue;

/* =========================
 * Pins
 * ========================= */

#define BUTTON_GPIO GPIOA
#define BUTTON_PORT PORTA
#define BUTTON_PIN  1U

#define ADC_BASE ADC0
#define ADC_CH_LIGHT       9U
#define ADC_CH_TEMPERATURE 12U

#define ADC_PIN_LIGHT      1U
#define ADC_PIN_TEMPERATURE 2U

#define LIGHT_THRESHOLD 2048
#define TEMP_THRESHOLD   2048

/* =========================
 * Prototypes
 * ========================= */

static void vTaskLightSensor(void *pvParameters);
static void vTaskTemperatureSensor(void *pvParameters);
static void vTaskButtonPolling(void *pvParameters);
static void vTaskSystemControl(void *pvParameters);

/* =========================
 * Main
 * ========================= */

int main(void)
{
    gpio_pin_config_t button_config =
    {
        kGPIO_DigitalInput,
        1,
    };

    adc16_config_t adc16ConfigStruct;

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    /* =========================
     * Button config - PTA1 pull-up
     * ========================= */

    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);

    PORT_SetPinMux(BUTTON_PORT, BUTTON_PIN, kPORT_MuxAsGpio);

    /*
     * Pull-up interno:
     * Pin fisico = 1 -> no presionado
     * Pin fisico = 0 -> presionado
     *
     * Conexion:
     * PTA1 ---- boton ---- GND
     */
    BUTTON_PORT->PCR[BUTTON_PIN] |= PORT_PCR_PE_MASK;
    BUTTON_PORT->PCR[BUTTON_PIN] |= PORT_PCR_PS_MASK;

    GPIO_PinInit(BUTTON_GPIO, BUTTON_PIN, &button_config);

    /* =========================
     * Analog pins
     * ========================= */

    /*
     * PTB1 -> ADC0_SE9
     * PTB2 -> ADC0_SE12
     *
     * Se limpian los PCR para usarlos como entradas analogicas.
     */
    PORTB->PCR[ADC_PIN_LIGHT] = 0x00000000;
    PORTB->PCR[ADC_PIN_TEMPERATURE] = 0x00000000;

    /* =========================
     * ADC config
     * ========================= */

    ADC16_GetDefaultConfig(&adc16ConfigStruct);
    adc16ConfigStruct.resolution = kADC16_ResolutionSE12Bit;
    adc16ConfigStruct.enableContinuousConversion = false;

    ADC16_Init(ADC_BASE, &adc16ConfigStruct);
    ADC16_EnableHardwareTrigger(ADC_BASE, false);
    ADC16_DoAutoCalibration(ADC_BASE);

    /* =========================
     * Queue config
     * ========================= */

    sensorQueue = xQueueCreate(10, sizeof(sensor_msg_t));

    if(sensorQueue == NULL)
    {
        PRINTF("Error creating queue\r\n");

        while(1)
        {
        }
    }

    PRINTF("FreeRTOS KL25Z - Parte 2 with Queues\r\n");
    PRINTF("Pot 1 Light -> PTB1 / ADC0_SE9\r\n");
    PRINTF("Pot 2 Temp  -> PTB2 / ADC0_SE12\r\n");
    PRINTF("Button      -> PTA1, inverted logic\r\n");
    PRINTF("Physical button: 1 = not pressed | 0 = pressed\r\n");
    PRINTF("Program button : 0 = not pressed | 1 = pressed\r\n\r\n");

    xTaskCreate(vTaskLightSensor,
                "Task1_Light",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                2,
                NULL);

    xTaskCreate(vTaskTemperatureSensor,
                "Task2_Temp",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                2,
                NULL);

    xTaskCreate(vTaskButtonPolling,
                "Task3_Button",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                1,
                NULL);

    xTaskCreate(vTaskSystemControl,
                "Task4_System",
                configMINIMAL_STACK_SIZE + 250,
                NULL,
                3,
                NULL);

    vTaskStartScheduler();

    while(1)
    {
    }
}

/* =========================
 * Task 1: Light Sensor
 * ========================= */

static void vTaskLightSensor(void *pvParameters)
{
    sensor_msg_t msg;
    adc16_channel_config_t adcConfigLight;

    adcConfigLight.channelNumber = ADC_CH_LIGHT;
    adcConfigLight.enableInterruptOnConversionCompleted = false;
    adcConfigLight.enableDifferentialConversion = false;

    while(1)
    {
        ADC16_SetChannelConfig(ADC_BASE, 0U, &adcConfigLight);

        while(0U == (kADC16_ChannelConversionDoneFlag &
                     ADC16_GetChannelStatusFlags(ADC_BASE, 0U)))
        {
        }

        msg.type = SENSOR_LIGHT;
        msg.value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

        xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* =========================
 * Task 2: Temperature Potentiometer
 * ========================= */

static void vTaskTemperatureSensor(void *pvParameters)
{
    sensor_msg_t msg;
    adc16_channel_config_t adcConfigTemperature;

    adcConfigTemperature.channelNumber = ADC_CH_TEMPERATURE;
    adcConfigTemperature.enableInterruptOnConversionCompleted = false;
    adcConfigTemperature.enableDifferentialConversion = false;

    while(1)
    {
        ADC16_SetChannelConfig(ADC_BASE, 0U, &adcConfigTemperature);

        while(0U == (kADC16_ChannelConversionDoneFlag &
                     ADC16_GetChannelStatusFlags(ADC_BASE, 0U)))
        {
        }

        msg.type = SENSOR_TEMP;
        msg.value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

        xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* =========================
 * Task 3: Button Polling
 * ========================= */

static void vTaskButtonPolling(void *pvParameters)
{
    sensor_msg_t msg;
    uint16_t physical_value = 1;

    while(1)
    {
        /*
         * Boton con logica invertida:
         * Pin fisico = 1 -> no presionado
         * Pin fisico = 0 -> presionado
         *
         * Valor enviado:
         * 0 -> no presionado
         * 1 -> presionado
         */
        physical_value = (uint16_t)GPIO_ReadPinInput(BUTTON_GPIO, BUTTON_PIN);

        msg.type = SENSOR_BUTTON;
        msg.value = (uint16_t)!physical_value;

        xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));

        /*
         * En Stage 2 todavia hay polling.
         * Esto se corrige en Stage 3 con interrupciones.
         */
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* =========================
 * Task 4: Receive Queue, Print and Control LEDs
 * ========================= */

static void vTaskSystemControl(void *pvParameters)
{
    sensor_msg_t msg;

    uint16_t light_value = 0;
    uint16_t temp_value = 0;
    uint16_t button_value = 0;

    while(1)
    {
        if(xQueueReceive(sensorQueue, &msg, portMAX_DELAY) == pdPASS)
        {
            switch(msg.type)
            {
                case SENSOR_LIGHT:
                    light_value = msg.value;
                    break;

                case SENSOR_TEMP:
                    temp_value = msg.value;
                    break;

                case SENSOR_BUTTON:
                    button_value = msg.value;
                    break;

                default:
                    break;
            }

            if(light_value < LIGHT_THRESHOLD)
            {
                LED_BLUE_ON();
            }
            else
            {
                LED_BLUE_OFF();
            }

            if(temp_value > TEMP_THRESHOLD)
            {
                LED_RED_ON();
            }
            else
            {
                LED_RED_OFF();
            }

            if(button_value == 1U)
            {
                LED_GREEN_ON();
            }
            else
            {
                LED_GREEN_OFF();
            }

            PRINTF("Light: %u | Temp: %u | Button: %u\r\n",
                   light_value,
                   temp_value,
                   button_value);
        }
    }
}
