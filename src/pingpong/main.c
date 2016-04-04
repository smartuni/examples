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
 #include <sys/types.h>
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
#include "getaddrinfo.h"
#else   //RIOT_VERSION
#include <pthread.h>
#include <netdb.h>
#endif  // RIOT_VERSION

// parameters
#define PP_BUF_SIZE         (16)
#define PP_MSG_QUEUE_SIZE   (8U)
#define PP_PORT_STR         "6414"
#define PP_IFID_STR         "4"

// function prototypes
static int ping(int argc, char **argv);
static int pong(char *addr_str);
//static int pp_send(char *addr_str, char *data);
static int pp_send(char *addr_str, char *data);
static void start_receiver(void);
static void *_receiver(void *arg);

#ifdef __RIOT__
extern int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res);

extern void freeaddrinfo(struct addrinfo *res);

extern const char *gai_strerror(int errcode);
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
    /* a mini shell to send pings */
    char cmd_buf[128];
    char cmd_ping[] = "ping";
    while(1) {
        puts("USAGE: ping [IP[%IF]]:");
        memset(cmd_buf, 0, 128);
        int cnt = 0;
        int lws = 0;
        int pos = 0;
        char *argv[2];
        int argc = 1;
        argv[0] = cmd_buf;
        argv[1] = NULL;
        while (cnt < 128) {
            int c = getchar();
            if (c=='\n' || c=='\r') {
                break;
            }
            if (c==' ') {
                if (cnt > lws+1) {
                    cmd_buf[cnt] = 0;
                    lws = cnt;
                    pos = cnt+1;
                    ++argc;
                }
            } else {
                cmd_buf[cnt] = (char)c;
            }
            ++cnt;
        }
        if (strlen(cmd_buf) < 4 || strcmp(cmd_ping, cmd_buf) != 0 || argc > 2)
            continue;
        if (argc > 1) {
            argv[1] = cmd_buf + pos;
        }
        ping(argc, argv);
    }
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
 * @return transfered bytes, or < 0 if failed
 */
static int pp_send(char *addr_str, char *data)
{
    int s;
    struct addrinfo hints, *res;
    int ret;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_DGRAM;

    if ((ret = getaddrinfo(addr_str, "6414", &hints, &res)) != 0) {
        puts("ERROR pp_send: getaddrinfo");
        return (-1);
    }

    // loop through all the results and make a socket
    if ((s=socket(res->ai_family,res->ai_socktype,res->ai_protocol)) < 0) {
        puts("ERROR pp_send: create socket");
        return (-1);
    }

    if ((ret=sendto(s,data,strlen(data),0,res->ai_addr,res->ai_addrlen)) < 0) {
        printf("ERROR pp_send: sending data: %d\n", ret);
    }
    freeaddrinfo(res);
    close(s);

    return ret;
}

/**
 * @brief start udp receiver thread
 */
static void start_receiver(void)
{
#ifdef __RIOT__
    thread_create(pp_stack, sizeof(pp_stack), THREAD_PRIORITY_MAIN,
                     THREAD_CREATE_STACKTEST, _receiver, NULL, "pingpong");
#else // __RIOT__
    pthread_create(&t_receiver, NULL, _receiver, NULL);
#endif // __RIOT__
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
    /* parse port */
    port = (uint16_t)atoi(PP_PORT_STR);
    if (port == 0) {
        puts("ERROR receiver: invalid port");
        return NULL;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    memset(&server_addr.sin6_addr, 0, sizeof(server_addr.sin6_addr));
    server_addr.sin6_port = htons(port);

    int s = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0) {
        puts("ERROR receiver: create socket");
        return NULL;
    }
    if (bind(s,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0) {
        puts("ERROR receiver: bind socket");
        close(s);
        return NULL;
    }
    printf(". started UDP server on port %" PRIu16 "\n", port);
    while (1) {
        int res;
        struct sockaddr_in6 src;
        memset(&src, 0, sizeof(src));
        memset(pp_buffer, 0, sizeof(pp_buffer));
        socklen_t src_len = sizeof(src);
        // blocking receive, waiting for data
        if ((res = recvfrom(s, pp_buffer, sizeof(pp_buffer), 0,
                            (struct sockaddr *)&src, &src_len)) < 0) {
            puts("ERROR receiver: recvfrom");
        }
        else if (res == 0) {
            puts("WARN receiver: Peer did shut down");
        }
        else { // check for PING or PONG
            inet_ntop(AF_INET6, &(src.sin6_addr),
                      src_addr_str, sizeof(src_addr_str));
            sprintf(src_addr_str, "%s%%"PP_IFID_STR, src_addr_str);
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
    close(s);
    return NULL;
}
