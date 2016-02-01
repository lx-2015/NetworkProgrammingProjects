// CSE5462 Sp2016 Lab3
// Group Member: Xiang Li, Haicheng Chen
// Date: 01/27/2016


#include "cse5462lib.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>

// This is the port that tcpd will always listen to
#define INNER_PORT 1040
#define LOCAL_IP "127.0.0.1"

// Implementation of SOCKET() using socket()
int SOCKET(int domain, int type, int protocol) {
	return socket(domain, type, protocol);
}

// Implementation of SEND() using sendto()
ssize_t SEND(int sockfd, const void *buf, size_t len, int flags) {
    ssize_t bytesSent = 0;
    // construct the local address for inner communication with tcpd
    struct sockaddr_in tcpdAddr;
    tcpdAddr.sin_family = AF_INET;
    tcpdAddr.sin_port = htons(INNER_PORT);
    tcpdAddr.sin_addr.s_addr = inet_addr(LOCAL_IP);
    memset(&(tcpdAddr.sin_zero), '\0', 8);
    // send!
    bytesSent = sendto(sockfd, buf, len, 0, (struct sockaddr*)&tcpAddr, sizeof(tcpAddr));
    
    return bytesSent;
}

// Implementation of RECV() using recvfrom()
ssize_t RECV(int sockfd, void *buf, size_t len, int flags) {
	// Construct the local address for inner communication with tcpd	
	struct sockaddr_in local_addr;
	local_addr.sin_famuily = AF_INET;
	local_addr.sin_port = htons(INNER_PORT);
	local_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
	memset(&(local_addr.sin_zero), '\0', 8);	
	// Inform the tcpd to send back data
	char start[1] = {1};	
	ssize_t sent = sendto(sockfd, (void *)start, 1, flags, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
	if (sent == -1) {
		perror("Error in RECV!");
	}		
	// Obtain data from tcpd
	ssize_t received = recvfrom(sockfd, buf, len, flags, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in));
	return received;
}

int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
	return 0;
}

int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	return 0;
}

