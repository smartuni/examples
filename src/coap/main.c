/**
 * written by smlng
 */

// standard
 #include <inttypes.h>
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
// network
 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <sys/socket.h>
 #include <unistd.h>
// riot
#include "board.h"
#include "periph/gpio.h"
#include "shell.h"
#include "thread.h"

#include "coap_example.h"

// array with available shell commands
static const shell_command_t shell_commands[] = {
    { "coap", "send get request", coap_cmd },
    { NULL, NULL, NULL }
};

extern void start_receiver(void);
extern int coap_cmd(int argc, char **argv);
/**
 * @brief the main programm loop
 *
 * @return non zero on error
 */
int main(void)
{
    // some initial infos
    puts("SmartUniversity - COAP Example!");
    puts("================");
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);
    puts("================");

    // start udp receiver
    start_receiver();

    // start shell
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    // should be never reached
    return 0;
}
