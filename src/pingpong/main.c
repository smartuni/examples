/**
 * written by smlng
 */

#include <stdio.h>
#include <inttypes.h>
#include "board.h"
#include "periph/gpio.h"

#include "kernel.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"
#include "shell.h"
#include "thread.h"

// some parameters
#define PP_MAX_PAYLOAD      (16)
#define PP_MSG_QUEUE_SIZE   (8U)
#define PP_PORT             (6414)

// function prototypes
static int ping(int argc, char **argv);
static void pong(char *addr_str);
static void send(char *addr_str, char *data);
static void start_receiver(void);
static void _parse(gnrc_pktsnip_t *pkt);
static void *_receiver(void *arg);

// array with available shell commands
static const shell_command_t shell_commands[] = {
    { "ping", "send multicast ping", ping },
    { NULL, NULL, NULL }
};
// thread stack
static char pp_stack[THREAD_STACKSIZE_MAIN];

/**
 * @brief the main programm loop
 */
int main(void)
{
    // some initial infos
    puts("SmartUniversity - PingPong Example!");
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

/**
 * @brief send a (multicast) ping
 *
 * @param argc: unused
 * @param argv: unused
 */
static int ping(int argc, char **argv)
{
    puts(". send PING to [ff02::1]");
    send("ff02::1", "PING");
    return 0;
}

/**
 * @brief send a (unicast) pong
 *
 * @param addr_str: unicast destination address
 */
static void pong(char *addr_str)
{
    printf(". send PONG to [%s].\n", addr_str);
    send(addr_str, "PONG");
}

/*
 * @brief generic udp send method
 *
 * @param addr_str: destination address
 * @param data: payload to send
 */
static void send(char *addr_str, char *data)
{
    ipv6_addr_t addr;
    uint8_t port[2];
    uint16_t temp = PP_PORT;
    port[0] = (uint8_t)temp;
    port[1] = temp >> 8;

    // parse destination address
    if (ipv6_addr_from_str(&addr, addr_str) == NULL) {
        puts("Error: unable to parse destination address");
        return;
    }
    gnrc_pktsnip_t *payload, *udp, *ip;
    // allocate payload
    payload = gnrc_pktbuf_add(NULL, data, strlen(data), GNRC_NETTYPE_UNDEF);
    if (payload == NULL) {
        puts("Error: unable to copy data to packet buffer");
        return;
    }
    // allocate UDP header
    udp = gnrc_udp_hdr_build(payload, port, 2, port, 2);
    if (udp == NULL) {
        puts("Error: unable to allocate UDP header");
        gnrc_pktbuf_release(payload);
        return;
    }
    // allocate IPv6 header
    ip = gnrc_ipv6_hdr_build(udp, NULL, 0, (uint8_t *)&addr, sizeof(addr));
    if (ip == NULL) {
        puts("Error: unable to allocate IPv6 header");
        gnrc_pktbuf_release(udp);
        return;
    }
    // send packet
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP, GNRC_NETREG_DEMUX_CTX_ALL, ip)) {
        puts("Error: unable to locate UDP thread");
        gnrc_pktbuf_release(ip);
        return;
    }
}

/**
 * @brief start udp receiver thread
 */
static void start_receiver(void)
{
    thread_create(pp_stack, sizeof(pp_stack), THREAD_PRIORITY_MAIN,
                     CREATE_STACKTEST, _receiver, NULL, "pingpong");
    puts(". started UDP server...");
}

/**
 * @brief parse udp packet content
 *
 * @param pkt: pointer to packet content
 */
static void _parse(gnrc_pktsnip_t *pkt)
{
    ipv6_hdr_t *ihdr;
    gnrc_pktsnip_t *snip = pkt;
    char src_addr_str[IPV6_ADDR_MAX_STR_LEN];
    char payload_str[PP_MAX_PAYLOAD];

    // parse headers and payload
    while (snip != NULL) {
        switch (snip->type) {
            case GNRC_NETTYPE_UNDEF:
                printf(". parse payload (%u Bytes)\n", snip->size);
                if (snip->size < PP_MAX_PAYLOAD) {
                    memcpy(payload_str, (char *)snip->data, snip->size);
                    payload_str[snip->size] = '\0';
                    printf(".. content: %s\n", payload_str);
                }
                break;
            case GNRC_NETTYPE_IPV6:
                puts(".. parse IPv6");
                ihdr = (ipv6_hdr_t *)snip->data;
                ipv6_addr_to_str(src_addr_str, &ihdr->src, sizeof(src_addr_str));
                break;
            case GNRC_NETTYPE_UDP:
                puts(".. parse UDP");
                break;
            default:
                break;
        }
        snip = snip->next;
    }
    // if packet had payload, check for PING or PONG
    if (strlen(payload_str) > 0) {
        LED_ON;
        if (strcmp(payload_str, "PING") == 0) {
            printf(". received PING from [%s].\n", src_addr_str);
            pong(src_addr_str);

        }
        if (strcmp(payload_str, "PONG") == 0) {
            printf(". received PONG from [%s].\n", src_addr_str);
        }
        LED_OFF;
    }
}

/**
 * @brief udp receiver thread function
 *
 * @param arg: unused
 */
static void *_receiver(void *arg)
{
    (void)arg;
    uint16_t port = PP_PORT;
    gnrc_netreg_entry_t pp_receiver;
    msg_t msg, reply;
    msg_t msg_queue[PP_MSG_QUEUE_SIZE];

    reply.content.value = (uint32_t)(-ENOTSUP);
    reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
    // setup the message queue
    msg_init_queue(msg_queue, PP_MSG_QUEUE_SIZE);

    // start receiver
    pp_receiver.pid = thread_getpid();
    pp_receiver.demux_ctx = (uint32_t)port;
    gnrc_netreg_register(GNRC_NETTYPE_UDP, &pp_receiver);

    while (1) {
        msg_receive(&msg);
        switch (msg.type) {
            case GNRC_NETAPI_MSG_TYPE_RCV:
                _parse((gnrc_pktsnip_t *)msg.content.ptr);
                break;
            case GNRC_NETAPI_MSG_TYPE_SND:
                break;
            case GNRC_NETAPI_MSG_TYPE_GET:
            case GNRC_NETAPI_MSG_TYPE_SET:
                msg_reply(&msg, &reply);
                break;
            default:
                break;
        }
    }
    // never reached
    return NULL;
}
