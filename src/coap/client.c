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

/* prototypes */
static int udp_send(char *addr_str, char *data);

/*
 * @brief parse coap shell command and execute
 */
int udp_cmd(int argc, char **argv)
{

}

/*
 * @brief generic udp send method
 *
 * @param[in] addr_str  destination address
 * @param[in] data      payload to send
 *
 * @return 0 on success, or 1 if failed
 */
static int udp_send(char *addr_str, char *data)
{
    struct sockaddr_in6 src, dst;
    size_t data_len = strlen(data);
    uint16_t port;
    int s;
    src.sin6_family = AF_INET6;
    dst.sin6_family = AF_INET6;
    memset(&src.sin6_addr, 0, sizeof(src.sin6_addr));
    /* parse destination address */
    if (inet_pton(AF_INET6, addr_str, &dst.sin6_addr) != 1) {
        puts("Error: unable to parse destination address");
        return 1;
    }
    /* parse port */
    port = (uint16_t)COAP_PORT;
    dst.sin6_port = htons(port);
    src.sin6_port = htons(port);
    s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        puts("error initializing socket");
        return 1;
    }
    if (sendto(s, data, data_len, 0, (struct sockaddr *)&dst, sizeof(dst)) < 0) {
        puts("error sending data");
    }
    close(s);
    return 0;
}
