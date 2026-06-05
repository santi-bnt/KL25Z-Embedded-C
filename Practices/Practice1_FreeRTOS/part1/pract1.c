/*
 * main.c - FreeRTOS KL25Z
 * Parte 1: Basic Concurrency
 * Tasks with global variables, polling, no queues, no mutexes, no interrupts
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

#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "fsl_adc16.h"

/* =========================
 * Global Variables
 * ========================= */

volatile uint16_t light_value = 0;
volatile uint16_t temp_value = 0;
volatile uint8_t button_state = 0;

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
static void vTaskLedControl(void *pvParameters);
static void vTaskSerialMonitor(void *pvParameters);

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

    PRINTF("FreeRTOS KL25Z - Parte 1 Basic Concurrency\r\n");
    PRINTF("Global variables + polling\r\n");
    PRINTF("Pot 1 Light -> PTB1 / ADC0_SE9\r\n");
    PRINTF("Pot 2 Temp  -> PTB2 / ADC0_SE12\r\n");
    PRINTF("Button      -> PTA1, inverted logic\r\n");
    PRINTF("Physical button: 1 = not pressed | 0 = pressed\r\n");
    PRINTF("Program button : 0 = not pressed | 1 = pressed\r\n\r\n");

    xTaskCreate(vTaskLightSensor,
                "Light",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                2,
                NULL);

    xTaskCreate(vTaskTemperatureSensor,
                "Temp",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                2,
                NULL);

    xTaskCreate(vTaskButtonPolling,
                "Button",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                1,
                NULL);

    xTaskCreate(vTaskLedControl,
                "LEDs",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                3,
                NULL);

    xTaskCreate(vTaskSerialMonitor,
                "Serial",
                configMINIMAL_STACK_SIZE + 200,
                NULL,
                1,
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

        light_value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* =========================
 * Task 2: Temperature Sensor
 * ========================= */

static void vTaskTemperatureSensor(void *pvParameters)
{
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

        temp_value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* =========================
 * Task 3: Button Polling
 * ========================= */

static void vTaskButtonPolling(void *pvParameters)
{
    uint8_t physical_value = 1;

    while(1)
    {
        /*
         * Boton con logica invertida:
         * Pin fisico = 1 -> no presionado
         * Pin fisico = 0 -> presionado
         *
         * Valor guardado:
         * 0 -> no presionado
         * 1 -> presionado
         */
        physical_value = (uint8_t)GPIO_ReadPinInput(BUTTON_GPIO, BUTTON_PIN);

        button_state = (uint8_t)!physical_value;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* =========================
 * Task 4: LED Control
 * ========================= */

static void vTaskLedControl(void *pvParameters)
{
    while(1)
    {
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

        if(button_state == 1U)
        {
            LED_GREEN_ON();
        }
        else
        {
            LED_GREEN_OFF();
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/* =========================
 * Task 5: Serial Monitor
 * ========================= */

static void vTaskSerialMonitor(void *pvParameters)
{
    while(1)
    {
        PRINTF("Light: %u | Temp: %u | Button: %u\r\n",
               light_value,
               temp_value,
               button_state);

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}