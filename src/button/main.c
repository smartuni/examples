/**
 * written by smlng
 */
#include <stdio.h>
#include "board.h"
#include "periph/gpio.h"

/**
 * @brief   callback function for button interrupt
 *
 * @param[in] arg   NULL
 */
static void btn_callback(void* arg)
{
    LED0_TOGGLE;
}

/**
 * @ brief  the main loop
 *
 * @return 1 on error
 */
int main(void)
{
    puts(" SmartUniversity - Button Example! ");
    puts("===================================");
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);
    puts("===================================");

    // init button as interrupt to trigger LED
    gpio_t pin = BUTTON_GPIO;
    gpio_mode_t mode = GPIO_IN_PU;
    gpio_flank_t flank = GPIO_FALLING;
    if (gpio_init_int(pin, mode, flank, btn_callback, NULL) < 0) {
        puts("Error while initializing  BUTTON_GPIO as external interrupt!");
        return 1;
    }
    printf("BUTTON_GPIO initialized successful as external interrupt.");

    // infinity loop 1
    while(1) {
        getchar();
    }

    return 0;
}
