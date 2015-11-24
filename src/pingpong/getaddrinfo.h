#ifndef PP_GETADDRINFO_H
#define PP_GETADDRINFO_H

#define ANY     (0)

struct addrinfo {
    int     ai_flags;
    int     ai_family;
    int     ai_socktype;
    int     ai_protocol;
    size_t  ai_addrlen;
    struct  sockaddr* ai_addr;
    char*   ai_canonname;     /* canonical name */
    struct  addrinfo* ai_next; /* this struct can form a linked list */
};

#endif // PP_GETADDRINFO_H
