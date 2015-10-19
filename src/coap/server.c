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

// static vars
static char coap_stack[THREAD_STACKSIZE_DEFAULT];
static int coap_socket = -1;
static char coap_buffer[COAP_BUF_SIZE];
static msg_t coap_msg_queue[COAP_MSG_QUEUE_SIZE];

/**
 * @brief start udp receiver thread
 */
static void start_receiver(void)
{
    thread_create(coap_stack, sizeof(coap_stack), THREAD_PRIORITY_MAIN,
                     CREATE_STACKTEST, _receiver, NULL, "coap_receiver");
    puts(". started coap receiver...");
}

/**
 * @brief udp receiver thread function
 *
 * @param[in] arg   unused
 */
static void *_receiver(void *arg)
{
    struct sockaddr_in6 server_addr;
    char src_addr_str[IPV6_ADDR_MAX_STR_LEN];
    uint16_t port;
    msg_init_queue(coap_msg_queue, COAP_MSG_QUEUE_SIZE);
    coap_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    /* parse port */
    port = (uint16_t)COAP_PORT;
    if (port == 0) {
        puts("Error: invalid port specified");
        return NULL;
    }
    server_addr.sin6_family = AF_INET6;
    memset(&server_addr.sin6_addr, 0, sizeof(server_addr.sin6_addr));
    server_addr.sin6_port = htons(port);
    if (coap_socket < 0) {
        puts("error initializing socket");
        coap_socket = 0;
        return NULL;
    }
    if (bind(coap_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        coap_socket = -1;
        puts("error binding socket");
        return NULL;
    }
    printf("Success: started UDP server on port %" PRIu16 "\n", port);
    while (1) {
        int res;
        struct sockaddr_in6 src;
        socklen_t src_len = sizeof(struct sockaddr_in6);
        // blocking receive, waiting for data
        if ((res = recvfrom(coap_socket, coap_buffer, sizeof(coap_buffer), 0,
                            (struct sockaddr *)&src, &src_len)) < 0) {
            puts("Error on receive");
        }
        else if (res == 0) {
            puts("Peer did shut down");
        }
        else { // check for PING or PONG
            inet_ntop(AF_INET6, &(src.sin6_addr),
                      src_addr_str, sizeof(src_addr_str));

            if (strcmp(coap_buffer, "PING") == 0) {
                printf(". received PING from [%s].\n", src_addr_str);
                pong(src_addr_str);
            }
            else if (strcmp(coap_buffer, "PONG") == 0) {
                printf(". received PONG from [%s].\n", src_addr_str);
            }
            else {
                printf(". received data from [%s] with %d bytes.\n", src_addr_str, res);
            }
        }
    }
    return NULL;
}
