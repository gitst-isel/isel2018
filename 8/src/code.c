#include <string.h>
#include <stdio.h>
#include "esp_common.h"
#include "gpio.h"
#include "freertos/task.h"
#include "fsm.h"

void gpio_config (GPIO_ConfigTypeDef *pGPIOConfig);
#define ETS_GPIO_INTR_ENABLE() _xt_isr_unmask ((1 << ETS_GPIO_INUM))

static int secret[] = { 1, 1, 1 };
static int code[10];
static int current = 0;
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
code_ok (fsm_t* fsm)
{
    return current == (sizeof(secret) / sizeof(secret[0]));
}

static int
timeout (fsm_t* fsm)
{
    return !code_ok(fsm) && started && (xTaskGetTickCount() > digit_deadline);
}

static int
timeout_and_digit_ok (fsm_t* fsm)
{
    return timeout(fsm) && (code[current] == secret[current]);
}

static int
timeout_and_digit_not_ok (fsm_t* fsm)
{
    return timeout(fsm) && (code[current] != secret[current]);
}

static int
button_pressed (fsm_t* fsm)
{
    return !code_ok(fsm) && !timeout(fsm) && button;
}

static void
increment_current_digit (fsm_t* fsm)
{
    code[current] ++;
    digit_deadline = xTaskGetTickCount() + (1000 / portTICK_RATE_MS);
    started = 1;
    button = 0;
}

static void
next_digit (fsm_t* fsm)
{
    code[++current] = 0;
    started = 0;
}

static void
reset (fsm_t* fsm)
{
    printf ("CODIGO: RESET %d (%d/%d, %d/%d, %d/%d)\n", current,
      code[0], secret[0], code[1], secret[1], code[2], secret[2]);
    current = 0;
    code[current] = 0;
    started = 0;
}

static void
accept_code (fsm_t* fsm)
{
    *valid_code = 1;
    reset (fsm);
    puts ("CODIGO: CORRECTO");
}

fsm_t*
fsm_new_code (int* validp, int button)
{
    static fsm_trans_t code_tt[] = {
        {  0, code_ok, 0, accept_code },
        {  0, timeout_and_digit_ok, 0, next_digit },
        {  0, timeout_and_digit_not_ok, 0, reset },
        {  0, button_pressed, 0, increment_current_digit },
        { -1, NULL, -1, NULL },
    };
    valid_code = validp;
    code_setup (button);
    return fsm_new (code_tt);
}
