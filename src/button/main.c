/**
 * written by smlng
 */
#include <stdio.h>
#include "board.h"
#include "periph/gpio.h"

// button parameters, matching Atmel SAM R21 board
#define BTN_PORT    0
#define BTN_PIN     28
#define BTN_PULL    GPIO_PULLUP
#define BTN_FLANK   GPIO_BOTH

/**
 * @brief callback function for button interrupt
 *
 * @param arg: contains gpio pin
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

// main
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
