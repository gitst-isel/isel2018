#include <string.h>
#include <stdio.h>
#include "esp_common.h"
#include "gpio.h"
#include "freertos/task.h"
#include "fsm.h"

void gpio_config (GPIO_ConfigTypeDef *pGPIOConfig);

#define LED 2

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;
    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

static void
button_setup (void)
{
    GPIO_ConfigTypeDef io_in_conf;
    io_in_conf.GPIO_IntrType = GPIO_PIN_INTR_DISABLE;
    io_in_conf.GPIO_Mode = GPIO_Mode_Input;
    io_in_conf.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_14;
    io_in_conf.GPIO_Pullup = GPIO_PullUp_EN;
    gpio_config (&io_in_conf);
}

static int
armed (fsm_t* fsm)
{
    return GPIO_INPUT_GET(14);
}

static int
disarmed (fsm_t* fsm)
{
    return ! GPIO_INPUT_GET(14);
}

static int
presence (fsm_t* fsm)
{
    return GPIO_INPUT_GET(12);
}

static void
alarm_on (fsm_t* fsm)
{
    GPIO_OUTPUT_SET (LED, 0);
}

static void
turn_on (fsm_t* fsm)
{
    GPIO_OUTPUT_SET (LED, 1);
}

static void
turn_off (fsm_t* fsm)
{
    GPIO_OUTPUT_SET (LED, 1);
}

static void
alarm (void* ignore)
{
    static fsm_trans_t alarm_tt[] = {
        {  0, armed, 1, turn_on },
        {  1, disarmed, 0, turn_off },
        {  1, presence, 1, alarm_on },
        { -1, NULL, -1, NULL },
    };
    fsm_t* alarm_fsm = fsm_new (alarm_tt);

    portTickType period =  250 /portTICK_RATE_MS;
    portTickType last = xTaskGetTickCount();
    while (1) {
        fsm_fire (alarm_fsm);
        vTaskDelayUntil (&last, period);
    }
}


void
user_init (void)
{
    xTaskHandle task_alarm;
    button_setup ();
    xTaskCreate (alarm, "alarm", 2048, NULL, 1, &task_alarm);
}
