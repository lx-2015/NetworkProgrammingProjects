// CSE5462 Sp2016 Lab3
// Group Member: Xiang Li, Haicheng Chen
// Date: 01/27/2016


#include "cse5462lib.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

// This is the port that tcpd will always listen to
#define INNER_PORT 1040
#define LOCAL_IP "127.0.0.1"
#define TCPD_PORT 1040
#define TCPD_IP "127.0.0.1"

// Helper function creates a socket and bind it for local communication
void local_socket(int *loc_sockfd, struct sockaddr_in* local_addr) {
	*loc_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (*loc_sockfd < 0) {
		perror("Error opening UDP socket");
		exit(1);
	}
	local_addr->sin_family = AF_INET;
	local_addr->sin_port = htons(INNER_PORT);
	local_addr->sin_addr.s_addr = inet_addr(LOCAL_IP);
	memset((void *)&(local_addr->sin_zero), '\0', 8);	
	socklen_t socklen = sizeof(struct sockaddr_in);
}

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
    tcpdAddr.sin_port = htons(1040);
    tcpdAddr.sin_addr.s_addr = inet_addr("164.107.113.22");
    memset(&(tcpdAddr.sin_zero), '\0', 8);
    // send!
    bytesSent = sendto(sockfd, buf, len, 0, (struct sockaddr*)&tcpdAddr, sizeof(tcpdAddr));
	fprintf(stderr, "%d bytes sent.\n", bytesSent);
    return bytesSent;
}

// Implementation of RECV() using recvfrom()
ssize_t RECV(int sockfd, void *buf, size_t len, int flags) {
	// Construct the local address for inner communication with tcpd	
	// The input sockfd is actually not used.
	int loc_sockfd = 0;
	struct sockaddr_in local_addr, back_addr;
	local_socket(&loc_sockfd, &local_addr);
	socklen_t socklen = sizeof(struct sockaddr_in);
	
	uint32_t rqstlen = len;
	//uint32_t rqstlen = htonl(len);
	// Copy request length to buffer bf in network order
	void *bf = malloc(sizeof(char) * 4);
	memcpy(bf, &rqstlen, 4);
	// Inform the tcpd to send back data by sending the amount of data it is requesting. 
	ssize_t sent = sendto(loc_sockfd, bf, 4, 0, (struct sockaddr *)&local_addr, socklen);
	if (sent < 0) {
		perror("Error in RECV to send initial data to tcpd server");
		exit(1);
	}
	free(bf);
	bf = NULL;
	// Obtain data from tcpd server
	ssize_t received = recvfrom(loc_sockfd, buf, len, flags, &back_addr, &socklen);
	return received;
}

int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
	return 0;
}

int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	return 0;
}

