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
#include "coap.h"

// parameters
#define COAP_BUF_SIZE         (63)
#define COAP_PORT             (5683)

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

    // init coap endpoints
    endpoint_setup();

    // start coap listener
    struct sockaddr_in6 server_addr;
    char src_addr_str[IPV6_ADDR_MAX_STR_LEN];
    uint16_t port;
    static int sock = -1;
    static uint8_t buf[COAP_BUF_SIZE];
    uint8_t scratch_raw[COAP_BUF_SIZE];
    coap_rw_buffer_t scratch_buf = {scratch_raw, sizeof(scratch_raw)};

    sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    /* parse port */
    port = (uint16_t)COAP_PORT;
    if (port == 0) {
        puts("Error: invalid port specified");
        return 1;
    }
    server_addr.sin6_family = AF_INET6;
    memset(&server_addr.sin6_addr, 0, sizeof(server_addr.sin6_addr));
    server_addr.sin6_port = htons(port);
    if (sock < 0) {
        puts("error initializing socket");
        sock = 0;
        return 1;
    }
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        sock = -1;
        puts("error binding socket");
        return 1;
    }
    printf("Success: started COAP listener on port %" PRIu16 "\n", port);
    while (1) {
        int res,rc;
        struct sockaddr_in6 src;
        socklen_t src_len = sizeof(struct sockaddr_in6);
        coap_packet_t pkt;
        // blocking receive, waiting for data
        if ((res = recvfrom(sock, buf, sizeof(buf), 0,
                            (struct sockaddr *)&src, &src_len)) < 0) {
            puts("Error on receive");
        }
        else if (res == 0) {
            puts("Peer did shut down");
        }
        else { // check for PING or PONG
            if (0 != (rc = coap_parse(&pkt, buf, res)))
                printf("Bad packet rc=%d\n", rc);
            else
            {
                inet_ntop(AF_INET6, &(src.sin6_addr),
                          src_addr_str, sizeof(src_addr_str));
                printf(". received COAP message from [%s].\n", src_addr_str);
                size_t rsplen = sizeof(buf);
                coap_packet_t rsppkt;
                coap_handle_req(&scratch_buf, &pkt, &rsppkt);

                if (0 != (rc = coap_build(buf, &rsplen, &rsppkt))) {
                    printf("coap_build failed rc=%d\n", rc);
                }
                else {
                    sendto(sock, buf, rsplen, 0, (struct sockaddr *)&src, src_len);
                }
            }
        }
    }

    // should be never reached
    return 0;
}
