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

#include "getaddrinfo.h"

void freeaddrinfo(struct addrinfo *ai)
{
    struct addrinfo *next;
    do {
        next = ai->ai_next;
        /* no need to free(ai->ai_addr) */
        free(ai);
    } while ((ai = next) != NULL);
}

const char *gai_strerror(int errcode)
{
    return "getaddrinfo error";
}

int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res)
{
    uint16_t port = 0;
    struct addrinfo *pai;
    struct sockaddr_in6 dst;

    if (node == NULL && service == NULL) {
        return 1;
    }

    if ((*res = (struct addrinfo *)malloc(sizeof(struct addrinfo) + sizeof(struct sockaddr_in6))) == NULL)
        return 1;

    pai = (struct addrinfo *)*res;
    pai->ai_flags = 0;
    pai->ai_family = AF_UNSPEC;
    pai->ai_socktype = ANY;
    pai->ai_protocol = ANY;
    pai->ai_addrlen = 0;
    pai->ai_canonname = NULL;
    pai->ai_addr = NULL;
    pai->ai_next = NULL;
    if (hints) {
        pai->ai_family = hints->ai_family;
        pai->ai_socktype = hints->ai_socktype;
    }

    if (service) {
        port = atoi(service);
        if (pai->ai_socktype == ANY) {
            /* caller accept *ANY* socktype */
            pai->ai_socktype = SOCK_DGRAM;
        }
        pai->ai_protocol = IPPROTO_UDP;
    }
    dst.sin6_port = htons(port);
    dst.sin6_scope_id = 0;

    if (node) {
        char *pch;
        char addr_str[IPV6_ADDR_MAX_STR_LEN+4];
        memset(addr_str,0,IPV6_ADDR_MAX_STR_LEN+4);
        memcpy(addr_str, node, strlen(node));
        if ( (pch=strchr(addr_str,'%')) != NULL ) {
            dst.sin6_scope_id = htonl(atoi(pch+1));
            addr_str[pch-addr_str] = 0;
        }
        if (inet_pton(AF_INET6, addr_str, &dst.sin6_addr) != 1) {
            return 1;
        }
    }
    pai->ai_addr = (struct  sockaddr *)&dst;

    memcpy(*res, pai, sizeof(struct addrinfo));
    (*res)->ai_addr = (struct sockaddr *)((*res) + 1);
    (*res)->ai_addrlen = sizeof(dst);
    memset((*res)->ai_addr, 0, pai->ai_addrlen);
    memcpy((*res)->ai_addr, &dst, sizeof(dst));
    (*res)->ai_addr->sa_family = (*res)->ai_family;
    return 0;
}
