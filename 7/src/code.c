#include <string.h>
#include <stdio.h>
#include "esp_common.h"
#include "gpio.h"
#include "freertos/task.h"
#include "fsm.h"

void gpio_config (GPIO_ConfigTypeDef *pGPIOConfig);
#define ETS_GPIO_INTR_ENABLE() _xt_isr_unmask ((1 << ETS_GPIO_INUM))

static portTickType digit_deadline = 0;

static int *valid_code = NULL;
static int GPIO_button = 14;

static volatile int button = 0;
static int started = 0;

static void
io_intr_handler (void)
{
  static portTickType debounce_timeout;
  portTickType now = xTaskGetTickCount();
  uint32 status = GPIO_REG_READ (GPIO_STATUS_ADDRESS);

  if (now >= debounce_timeout) {
      debounce_timeout = now + (200 /portTICK_RATE_MS);
      button = 1;
  }

  /* rearm interrupts */
  GPIO_REG_WRITE (GPIO_STATUS_W1TC_ADDRESS, status);
}

void
code_setup (int button)
{
  GPIO_ConfigTypeDef io_in_conf;

  GPIO_button = button;

  io_in_conf.GPIO_IntrType = GPIO_PIN_INTR_NEGEDGE;
  io_in_conf.GPIO_Mode = GPIO_Mode_Input;
  io_in_conf.GPIO_Pin = (1 << GPIO_button);
  io_in_conf.GPIO_Pullup = GPIO_PullUp_EN;
  gpio_config (&io_in_conf);
  gpio_intr_handler_register ((void *) io_intr_handler, NULL);
  ETS_GPIO_INTR_ENABLE();
}

static int
timeout (fsm_t* fsm)
{
    return xTaskGetTickCount() > digit_deadline;
}

static int
button_pressed (fsm_t* fsm)
{
    return button && !timeout(fsm);
}

static void
accept_code (fsm_t* fsm)
{
    *valid_code = 1;
    puts ("CODIGO: CORRECTO");
}

fsm_t*
fsm_new_code (int* validp, int button)
{
    /* code = 123 */
    static fsm_trans_t code_tt[] = {
        {  0, button_pressed, 1, NULL }, /* 1 */
        {  1, timeout,        2, NULL },
        {  2, button_pressed, 3, NULL }, /* 1 */
        {  3, button_pressed, 4, NULL }, /* 2 */
        {  4, timeout,        5, NULL },
        {  5, button_pressed, 6, NULL }, /* 1 */
        {  6, button_pressed, 7, NULL }, /* 2 */
        {  7, button_pressed, 8, NULL }, /* 3 */
        {  8, timeout,        0, accept_code },

        {  1, button_pressed, 0, NULL },
        {  2, timeout,        0, NULL },
        {  3, timeout,        0, NULL },
        {  4, button_pressed, 0, NULL },
        {  5, timeout,        0, NULL },
        {  6, timeout,        0, NULL },
        {  7, timeout,        0, NULL },
        {  8, button_pressed, 0, NULL },

        { -1, NULL, -1, NULL },
    };
    valid_code = validp;
    code_setup (button);
    return fsm_new (code_tt);
}
