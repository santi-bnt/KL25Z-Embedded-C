/*
 * main.c - FreeRTOS KL25Z
 * Example: tasks with sensors without queues or interruptions
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

#define BUTTON_PORT GPIOB
#define BUTTON_PIN 0U /* PTB0 */

#define ADC_BASE ADC0
#define ADC_CH_LIGHT 9U         /* PTB1 / ADC0_SE9 */
#define ADC_CH_TEMPERATURE 12U  /* PTB2 / ADC0_SE12 */

/* Example thresholds */
#define LIGHT_THRESHOLD 2048
#define TEMP_THRESHOLD 2048

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
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    /* =========================
     * BUTTON CONFIG - PTB0
     * ========================= */

    gpio_pin_config_t button_config =
    {
        kGPIO_DigitalInput,
        0,
    };

    CLOCK_EnableClock(kCLOCK_PortB);

    PORT_SetPinMux(PORTB, BUTTON_PIN, kPORT_MuxAsGpio);

    /*
     * Pull-down interno:
     *
     * Logica:
     * 0 = no presionado
     * 1 = presionado
     *
     * Conexion:
     * PTB0 ---- boton ---- 3.3V
     */
    PORTB->PCR[BUTTON_PIN] |= PORT_PCR_PE_MASK;
    PORTB->PCR[BUTTON_PIN] &= ~PORT_PCR_PS_MASK;

    GPIO_PinInit(BUTTON_PORT, BUTTON_PIN, &button_config);

    /* =========================
     * ADC CONFIG
     * ========================= */

    adc16_config_t adc16ConfigStruct;

    ADC16_GetDefaultConfig(&adc16ConfigStruct);
    adc16ConfigStruct.resolution = kADC16_ResolutionSE12Bit;

    adc16ConfigStruct.enableContinuousConversion = false;

    ADC16_Init(ADC_BASE, &adc16ConfigStruct);
    ADC16_EnableHardwareTrigger(ADC_BASE, false);
    ADC16_DoAutoCalibration(ADC_BASE);

    PRINTF("FreeRTOS KL25Z - Tasks with sensors without queues or interruptions\r\n");
    PRINTF("Button PTB0 configured with internal pull-down\r\n");
    PRINTF("Button logic: 0 = not pressed | 1 = pressed\r\n\r\n");

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

    while (1)
    {
    }
}

/* =========================
 * Tasks
 * ========================= */

static void vTaskLightSensor(void *pvParameters)
{
    adc16_channel_config_t adcConfigLight;

    adcConfigLight.channelNumber = ADC_CH_LIGHT;
    adcConfigLight.enableInterruptOnConversionCompleted = false;
    adcConfigLight.enableDifferentialConversion = false;

    while (1)
    {
        ADC16_SetChannelConfig(ADC_BASE, 0U, &adcConfigLight);

        while (0U == (kADC16_ChannelConversionDoneFlag &
                      ADC16_GetChannelStatusFlags(ADC_BASE, 0U)))
        {
        }

        light_value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void vTaskTemperatureSensor(void *pvParameters)
{
    adc16_channel_config_t adcConfigTemperature;

    adcConfigTemperature.channelNumber = ADC_CH_TEMPERATURE;
    adcConfigTemperature.enableInterruptOnConversionCompleted = false;
    adcConfigTemperature.enableDifferentialConversion = false;

    while (1)
    {
        ADC16_SetChannelConfig(ADC_BASE, 0U, &adcConfigTemperature);

        while (0U == (kADC16_ChannelConversionDoneFlag &
                      ADC16_GetChannelStatusFlags(ADC_BASE, 0U)))
        {
        }

        temp_value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void vTaskButtonPolling(void *pvParameters)
{
    for (;;)
    {
        /*
         * Con pull-down:
         * 0 = no presionado
         * 1 = presionado
         */
        button_state = GPIO_ReadPinInput(BUTTON_PORT, BUTTON_PIN);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void vTaskLedControl(void *pvParameters)
{
    while (1)
    {
        if (light_value < LIGHT_THRESHOLD)
        {
            LED_BLUE_ON();
        }
        else
        {
            LED_BLUE_OFF();
        }

        if (temp_value > TEMP_THRESHOLD)
        {
            LED_RED_ON();
        }
        else
        {
            LED_RED_OFF();
        }

        /*
         * Boton:
         * 1 = presionado
         * 0 = no presionado
         */
        if (button_state == 1)
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

static void vTaskSerialMonitor(void *pvParameters)
{
    while (1)
    {
        PRINTF("Light: %u | Temp: %u | Button: %u\r\n",
               light_value,
               temp_value,
               button_state);

        vTaskDelay(pdMS_TO_TICKS(1500));
    }
}
