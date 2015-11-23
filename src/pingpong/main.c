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
// check for riot
#ifdef RIOT_VERSION

#ifndef __RIOT__
#define __RIOT__
#endif // __RIOT__

#include "board.h"
#include "periph/gpio.h"
#include "shell.h"
#include "thread.h"

#else   //RIOT_VERSION
#include <pthread.h>
#endif  // RIOT_VERSION

// parameters
#define PP_BUF_SIZE         (16)
#define PP_MSG_QUEUE_SIZE   (8U)
#define PP_PORT             (6414)

// function prototypes
static int ping(int argc, char **argv);
static int pong(char *addr_str);
static int pp_send(char *addr_str, char *data);
static void start_receiver(void);
static void *_receiver(void *arg);

#ifdef __RIOT__
// array with available shell commands
static const shell_command_t shell_commands[] = {
    { "ping", "send multicast ping", ping },
    { NULL, NULL, NULL }
};
static msg_t pp_msg_queue[PP_MSG_QUEUE_SIZE];
static char pp_stack[THREAD_STACKSIZE_DEFAULT];
#else
static pthread_t t_receiver;
#define IPV6_ADDR_MAX_STR_LEN INET6_ADDRSTRLEN
#endif // __RIOT__

static char pp_buffer[PP_BUF_SIZE];

/**
 * @brief the main programm loop
 *
 * @return non zero on error
 */
int main(void)
{
    // some initial infos
    puts("SmartUniversity - PingPong Example!");
    puts("================");
#ifdef __RIOT__
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    printf("This board features a(n) %s MCU.\n", RIOT_MCU);
#else // __RIOT__
    printf("You are running Linux, MacOS, or FreeBSD?\n");
#endif // __RIOT__
    puts("================");

    // start udp receiver
    start_receiver();
#ifdef __RIOT__
    // start shell
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
#else  // __RIOT__
    char cmd[] = "ping ff02::1%4";
    char *pos = cmd;
    char *argv[3];
    int argc = 1;
    argv[0] = pos;
    argv[1] = NULL;
    argv[2] = NULL;
    if ( (pos=strchr(cmd, ' ')) != NULL ) {
        *pos = 0;
        argv[1] = pos+1;
        argc = 2;
    }
    ping(argc, argv);
    while(1)
        getchar();
#endif // __RIOT__
    // should be never reached
    return 0;
}

/**
 * @brief send a (multicast) ping
 *
 * @param[in] argc  unused
 * @param[in] argv  unused
 *
 * @return 0 on success, or 1 if failed
 */
static int ping(int argc, char **argv)
{
    int ret = -1;
    if (argc==2) {
        printf(". send user PING to [%s]\n", argv[1]);
        ret = pp_send(argv[1], "PING");
    }
    else {
        puts(". send multicast PING to [ff02::1]");
        ret = pp_send("ff02::1", "PING");
    }
    return ret;
}

/**
 * @brief send a (unicast) pong
 *
 * @param[in] addr_str  unicast destination address
 *
 * @return 0 on success, or 1 if failed
 */
static int pong(char *addr_str)
{
    printf(". send PONG to [%s].\n", addr_str);
    int ret = pp_send(addr_str, "PONG");
    return ret;
}

/*
 * @brief generic udp send method
 *
 * @param[in] addr_str  destination address
 * @param[in] data      payload to send
 *
 * @return 0 on success, or 1 if failed
 */
static int pp_send(char *addr_str, char *data)
{
    struct sockaddr_in6 dst;
    size_t data_len = strlen(data);
    uint16_t port;
    int s;
    dst.sin6_family = AF_INET6;
    /* parse destination address */
    char *pch;
    if ( (pch=strchr(addr_str,'%')) != NULL ) {
        dst.sin6_scope_id = atoi(pch+1);
        addr_str[pch-addr_str] = '\0';
    }
    if (inet_pton(AF_INET6, addr_str, &dst.sin6_addr) != 1) {
        puts("ERROR pp_send: parse destination address");
        return 1;
    }
    /* parse port */
    port = (uint16_t)PP_PORT;
    dst.sin6_port = htons(port);
    s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        puts("ERROR pp_send: initializing socket");
        return 1;
    }
    if (sendto(s, data, data_len, 0, (struct sockaddr *)&dst, sizeof(dst)) < 0) {
        puts("ERROR pp_send: sending data");
    }
    close(s);
    return 0;
}

/**
 * @brief start udp receiver thread
 */
static void start_receiver(void)
{
#ifdef __RIOT__
    thread_create(pp_stack, sizeof(pp_stack), THREAD_PRIORITY_MAIN,
                     CREATE_STACKTEST, _receiver, NULL, "pingpong");
#else // __RIOT__
    pthread_create(&t_receiver, NULL, _receiver, NULL);
#endif // __RIOT__
    puts(". started UDP server...");
}

/**
 * @brief udp receiver thread function
 *
 * @param[in] arg   unused
 */
static void *_receiver(void *arg)
{
    (void) arg;

    struct sockaddr_in6 server_addr;
    char src_addr_str[IPV6_ADDR_MAX_STR_LEN+4];
    uint16_t port;
#ifdef __RIOT__
    msg_init_queue(pp_msg_queue, PP_MSG_QUEUE_SIZE);
#endif // __RIOT__
    int pp_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    /* parse port */
    port = (uint16_t)PP_PORT;
    if (port == 0) {
        puts("ERROR receiver: invalid port");
        return NULL;
    }
    server_addr.sin6_family = AF_INET6;
    memset(&server_addr.sin6_addr, 0, sizeof(server_addr.sin6_addr));
    server_addr.sin6_port = htons(port);
    if (pp_socket < 0) {
        puts("ERROR receiver: create socket");
        pp_socket = 0;
        return NULL;
    }
    if (bind(pp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        pp_socket = -1;
        puts("ERROR receiver: bind socket");
        return NULL;
    }
    printf("Success: started UDP server on port %" PRIu16 "\n", port);
    while (1) {
        int res;
        struct sockaddr_in6 src;
        socklen_t src_len = sizeof(struct sockaddr_in6);
        // blocking receive, waiting for data
        if ((res = recvfrom(pp_socket, pp_buffer, sizeof(pp_buffer), 0,
                            (struct sockaddr *)&src, &src_len)) < 0) {
            puts("ERROR receiver: recvfrom");
        }
        else if (res == 0) {
            puts("WARN receiver: Peer did shut down");
        }
        else { // check for PING or PONG
            inet_ntop(AF_INET6, &(src.sin6_addr),
                      src_addr_str, sizeof(src_addr_str));
            sprintf(src_addr_str, "%s%%%" PRIu32, src_addr_str, src.sin6_scope_id);
            if (strcmp(pp_buffer, "PING") == 0) {
                printf(". received PING from [%s].\n", src_addr_str);
                pong(src_addr_str);
            }
            else if (strcmp(pp_buffer, "PONG") == 0) {
                printf(". received PONG from [%s].\n", src_addr_str);
            }
            else {
                printf(". received data from [%s] with %d bytes.\n", src_addr_str, res);
            }
        }
    }
    return NULL;
}
