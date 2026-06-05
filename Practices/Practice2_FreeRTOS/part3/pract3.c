/*
 * main.c - FreeRTOS KL25Z
 * Stage 3: Event-Driven Architecture with Mutexes and Interrupts
 *
 * Fotoresistencia -> PTB1 / ADC0_SE9
 * Sensor temp/pot  -> PTB2 / ADC0_SE12
 * Boton           -> PTA1 / PORTA interrupt
 *
 * Boton con logica invertida:
 * PTA1 ---- boton ---- GND
 * Se usa pull-up interno.
 *
 * Sensor analogico de temperatura o potenciometro:
 * Extremo 1 -> 3.3V
 * Extremo 2 -> GND
 * Centro    -> PTB2 / ADC0_SE12
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

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
 * FreeRTOS Objects
 * ========================= */

QueueHandle_t sensorQueue;
SemaphoreHandle_t xAdcMutex;
SemaphoreHandle_t xButtonSemaphore;

/* =========================
 * Pins and Thresholds
 * ========================= */

#define BUTTON_GPIO GPIOA
#define BUTTON_PORT PORTA
#define BUTTON_PIN  1U

#define ADC_BASE ADC0

#define ADC_CH_LIGHT       9U
#define ADC_CH_TEMPERATURE 12U

#define ADC_PIN_LIGHT      1U
#define ADC_PIN_TEMPERATURE 2U

#define LIGHT_THRESHOLD 2048U
#define TEMP_THRESHOLD  2048U

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

    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);

    /* =========================
     * Button config - PTA1 interrupt
     * ========================= */

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
     * Analog inputs
     * ========================= */

    /*
     * PTB1 -> ADC0_SE9  -> Fotoresistencia
     * PTB2 -> ADC0_SE12 -> Sensor analogico de temperatura o potenciometro
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
     * FreeRTOS sync objects
     * ========================= */

    sensorQueue = xQueueCreate(10, sizeof(sensor_msg_t));
    xAdcMutex = xSemaphoreCreateMutex();
    xButtonSemaphore = xSemaphoreCreateBinary();

    if((sensorQueue == NULL) ||
       (xAdcMutex == NULL) ||
       (xButtonSemaphore == NULL))
    {
        PRINTF("Error creating FreeRTOS objects\r\n");

        while(1)
        {
        }
    }

    /* =========================
     * Configure button interrupt
     * ========================= */

    PORT_ClearPinsInterruptFlags(BUTTON_PORT, (1U << BUTTON_PIN));
    PORT_SetPinInterruptConfig(BUTTON_PORT, BUTTON_PIN, kPORT_InterruptEitherEdge);

    NVIC_SetPriority(PORTA_IRQn, 3U);
    EnableIRQ(PORTA_IRQn);

    PRINTF("FreeRTOS KL25Z - Stage 3 Event Driven\r\n");
    PRINTF("ADC0 protected with xAdcMutex\r\n");
    PRINTF("Button uses PORTA interrupt + binary semaphore\r\n");
    PRINTF("Light sensor -> PTB1 / ADC0_SE9\r\n");
    PRINTF("Temperature analog input -> PTB2 / ADC0_SE12\r\n");
    PRINTF("Button -> PTA1 / PORTA IRQ, inverted logic\r\n\r\n");

    xTaskCreate(vTaskLightSensor,
                "Task_Light",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                2,
                NULL);

    xTaskCreate(vTaskTemperatureSensor,
                "Task_Temp",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                2,
                NULL);

    xTaskCreate(vTaskButtonPolling,
                "Task_Button",
                configMINIMAL_STACK_SIZE + 100,
                NULL,
                1,
                NULL);

    xTaskCreate(vTaskSystemControl,
                "Task_System",
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
 * ADC protected with Mutex
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
        if(xSemaphoreTake(xAdcMutex, portMAX_DELAY) == pdTRUE)
        {
            ADC16_SetChannelConfig(ADC_BASE, 0U, &adcConfigLight);

            while(0U == (kADC16_ChannelConversionDoneFlag &
                         ADC16_GetChannelStatusFlags(ADC_BASE, 0U)))
            {
            }

            msg.type = SENSOR_LIGHT;
            msg.value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

            xSemaphoreGive(xAdcMutex);

            xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* =========================
 * Task 2: Temperature Analog Sensor
 * ADC protected with Mutex
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
        if(xSemaphoreTake(xAdcMutex, portMAX_DELAY) == pdTRUE)
        {
            ADC16_SetChannelConfig(ADC_BASE, 0U, &adcConfigTemperature);

            while(0U == (kADC16_ChannelConversionDoneFlag &
                         ADC16_GetChannelStatusFlags(ADC_BASE, 0U)))
            {
            }

            msg.type = SENSOR_TEMP;
            msg.value = ADC16_GetChannelConversionValue(ADC_BASE, 0U);

            xSemaphoreGive(xAdcMutex);

            xQueueSend(sensorQueue, &msg, pdMS_TO_TICKS(10));
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/* =========================
 * Task 3: Button Event Task
 * Zero CPU while waiting
 * ========================= */

static void vTaskButtonPolling(void *pvParameters)
{
    sensor_msg_t msg;
    uint16_t physical_value = 1;

    while(1)
    {
        /*
         * Esta tarea duerme indefinidamente.
         * Solo despierta cuando PORTA_IRQHandler libera el semaforo.
         */
        if(xSemaphoreTake(xButtonSemaphore, portMAX_DELAY) == pdTRUE)
        {
            /*
             * Debounce por software.
             */
            vTaskDelay(pdMS_TO_TICKS(50));

            /*
             * Logica invertida:
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
             * Limpia rebotes acumulados durante el debounce.
             */
            while(xSemaphoreTake(xButtonSemaphore, 0) == pdTRUE)
            {
            }
        }
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

    TickType_t lastPrintTime = xTaskGetTickCount();

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

            /*
             * LED azul: luz.
             * Si funciona al reves, cambia < por >.
             */
            if(light_value < LIGHT_THRESHOLD)
            {
                LED_BLUE_ON();
            }
            else
            {
                LED_BLUE_OFF();
            }

            /*
             * LED rojo: valor analogico de temperatura alto.
             */
            if(temp_value > TEMP_THRESHOLD)
            {
                LED_RED_ON();
            }
            else
            {
                LED_RED_OFF();
            }

            /*
             * LED verde: boton presionado.
             */
            if(button_value == 1U)
            {
                LED_GREEN_ON();
            }
            else
            {
                LED_GREEN_OFF();
            }

            if((xTaskGetTickCount() - lastPrintTime) >= pdMS_TO_TICKS(1500))
            {
                PRINTF("Light ADC: %u | Temp ADC: %u | Button: %u\r\n",
                       light_value,
                       temp_value,
                       button_value);

                lastPrintTime = xTaskGetTickCount();
            }
        }
    }
}

/* =========================
 * PORTA ISR for button
 * ========================= */

void PORTA_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    PORT_ClearPinsInterruptFlags(BUTTON_PORT, (1U << BUTTON_PIN));

    if(xButtonSemaphore != NULL)
    {
        xSemaphoreGiveFromISR(xButtonSemaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}