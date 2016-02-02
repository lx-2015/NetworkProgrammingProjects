#ifndef CSE5462LIB_H
#define CSE5462LIB_H

#include <sys/types.h>
#include <sys/socket.h>

int SOCKET(int domain, int type, int protocol);
ssize_t SEND(int sockfd, const void *buf, size_t len, int flags);
ssize_t RECV(int sockfd, void *buf, size_t len, int flags);
int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen); 

#endif
