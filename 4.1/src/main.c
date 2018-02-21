#include <string.h>
#include <stdio.h>
#include "esp_common.h"
#include "gpio.h"
#include "freertos/task.h"
#include "fsm.h"

void gpio_config (GPIO_ConfigTypeDef *pGPIOConfig);
#define ETS_GPIO_INTR_ENABLE() _xt_isr_unmask ((1 << ETS_GPIO_INUM))

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


static volatile int button = 0;

static void
io_intr_handler (void)
{
    static portTickType debounce_timeout;
    portTickType now = xTaskGetTickCount();
    uint32 status = GPIO_REG_READ (GPIO_STATUS_ADDRESS);
    /* status & GPIO_Pin_4  or  status & GPIO_Pin_5 */

    if (now >= debounce_timeout) {
        debounce_timeout = now + (500 /portTICK_RATE_MS);
        button = 1;
    }

    /* rearm interrupts */
    GPIO_REG_WRITE (GPIO_STATUS_W1TC_ADDRESS, status);
}

static void
button_setup (void)
{
    GPIO_ConfigTypeDef io_in_conf;
    io_in_conf.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
    io_in_conf.GPIO_Mode = GPIO_Mode_Input;
    io_in_conf.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
    io_in_conf.GPIO_Pullup = GPIO_PullUp_EN;
    gpio_config (&io_in_conf);
    gpio_intr_handler_register ((void *) io_intr_handler, NULL);
    ETS_GPIO_INTR_ENABLE();
}

static int
button_pressed (fsm_t* fsm)
{
    return button;
}

static void
turn_on (fsm_t* fsm)
{
    GPIO_OUTPUT_SET (LED, 0);
    button = 0;
}

static void
turn_off (fsm_t* fsm)
{
    GPIO_OUTPUT_SET (LED, 1);
    button = 0;
}

static void
lampara (void* ignore)
{
    static fsm_trans_t lampara_tt[] = {
        {  0, button_pressed, 1, turn_on },
        {  1, button_pressed, 0, turn_off },
        { -1, NULL, -1, NULL },
    };
    fsm_t* lampara_fsm = fsm_new (lampara_tt);

    portTickType period =  250 /portTICK_RATE_MS;
    portTickType last = xTaskGetTickCount();
    while (1) {
        fsm_fire (lampara_fsm);
        vTaskDelayUntil (&last, period);
    }
}


void
user_init (void)
{
    xTaskHandle task_lampara;
    button_setup ();
    xTaskCreate (lampara, "lampara", 2048, NULL, 1, &task_lampara);
}
