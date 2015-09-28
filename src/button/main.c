/**
 * written by smlng
 */
#include <stdio.h>
#include "board.h"
#include "periph/gpio.h"

// compatibility
#ifndef LED_ON
#define LED_ON      LED_RED_ON
#define LED_OFF     LED_RED_OFF
#define LED_TOGGLE  LED_RED_TOGGLE
#endif

// button parameters, matching Atmel SAM R21 board
#ifdef BOARD_SAMR21_XPRO
#define BTN_PORT    0   // Port+Pin A28
#define BTN_PIN     28
#define BTN_PULL    GPIO_PULLUP
#define BTN_FLANK   GPIO_BOTH
#endif
// or button parameters, matching Phytec phyWAVE
// Note: port is ignored, only pin is used by GPIO
#ifdef BOARD_PBA_D_01_KW2X
#define BTN_PORT    0   // Port+Pin D1
#define BTN_PIN     3
#define BTN_PULL    GPIO_PULLUP
#define BTN_FLANK   GPIO_BOTH
#endif

/**
 * @brief   callback function for button interrupt
 *
 * @param[in] arg   contains gpio pin
 */
static void btn_callback(void* arg)
{
    gpio_t pin = *((gpio_t*)arg);
    if (gpio_read(pin)) {
        puts("LED off");
        LED_OFF;
    }
    else {
        puts("LED on");
        LED_ON;
    }
}

/**
 * @ brief  the main loop
 *
 * @return 1 on error
 */
int main(void)
{
    puts("SmartUniversity - Button Example!");
    puts("================");
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);
    puts("================");

    // init button as interrupt to trigger LED
    int gpio_pin = GPIO(BTN_PORT, BTN_PIN);
    if (gpio_init_int(gpio_pin, BTN_PULL, BTN_FLANK, btn_callback, (void *)&gpio_pin) < 0) {
        printf("Error while initializing  PORT_%i.%02i as external interrupt\n",
               BTN_PORT, BTN_PIN);
        return 1;
    }
    printf("PORT_%i.%02i initialized successful as external interrupt\n",
           BTN_PORT, BTN_PIN);

    // infinity loop 1
    while(1) {
        getchar();
    }

    return 0;
}
