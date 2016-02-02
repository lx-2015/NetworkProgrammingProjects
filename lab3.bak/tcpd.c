// CSE5462 Sp2016 Lab3
// Group member: Xiang Li, Haicheng Chen
// Date: 01/27/2016

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <strings.h>

// This is the port tahtthat tcpd will always listen to
#define INNER_PORT 1040
#define LOCAL_IP "127.0.0.1"
#define TROLL_PORT 1050
#define TROLL_IP "164.107.113.20" /* gamma.cse.ohio-state.edu */
#define SERVER_IP "164.107.113.23" /* eta.cse.ohio-state.edu */
#define SERVER_PORT 1060
#define MSS 1000


// Helper function creates a socket and bind it for local communication
void local_socket(int *loc_sockfd, struct sockaddr_in *local_addr) {
	*loc_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (*loc_sockfd < 0) {
		perror("Error opening UDP socket in tcpd");
		exit(1);
	}
	local_addr->sin_family = AF_INET;
	local_addr->sin_port = htons(INNER_PORT);
	local_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	memset((void *)&(local_addr->sin_zero), '\0', 8);	
	socklen_t socklen = sizeof(struct sockaddr_in);
	if (bind(*loc_sockfd, (struct sockaddr *)local_addr, socklen) == -1) {
		perror("Error in tcpd to bind socket");
		exit(1);
	}	
}

// Helper function creates a socket and bind it for communication with troll
void troll_socket(int *tro_sockfd, struct sockaddr_in *troll_addr) {
	*tro_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (*tro_sockfd < 0) {
		perror("Error opening UDP socket in tcpd");
		exit(1);
	}
	troll_addr->sin_family = AF_INET;
	troll_addr->sin_port = htons(TROLL_PORT);
	troll_addr->sin_addr.s_addr = htonl(INADDR_ANY);
	memset((void *)&(troll_addr->sin_zero), '\0', 8);	
	socklen_t socklen = sizeof(struct sockaddr_in);
	if (bind(*tro_sockfd, (struct sockaddr *)troll_addr, socklen) == - 1) {
		perror("Error in tcpd to bind socket");
		exit(1);
	}
}

// Implementation of the server side of tcpd
void server() {
	// Open a socket for communicating with ftps
	int loc_sockfd = 0;
	struct sockaddr_in local_addr;
	local_socket(&loc_sockfd, &local_addr);
	socklen_t socklen = sizeof(struct sockaddr_in);	

	// Open another socket for communicating and obtain the data from troll
	int tro_sockfd = 0;
	struct sockaddr_in troll_addr;
	troll_socket(&tro_sockfd, &troll_addr);
	while (1) {
		// ftps will first send the request packet size to inform tcpd that it is ready to receive data.
		void *bf = malloc(sizeof(char) * 4);
		ssize_t rcvd = recvfrom(loc_sockfd, bf, 4, MSG_WAITALL, NULL, NULL);
		if (rcvd < 0) {
			perror("Error in tcpd server to receive data from ftps!");
			close(loc_sockfd);
			close(tro_sockfd);
			exit(1);
		}
		uint32_t rqstlen = *((uint32_t *)bf);
		//uint32_t rqstlen = ntohl(*((uint32_t *)bf));
		free(bf);
		bf = NULL;		
		// Receive a packet from troll
		void *buf = malloc(sizeof(char) * MSS);
		//receive_all(tro_sockfd, buf, MSS, 0, NULL, NULL);
		socklen_t trolen = sizeof(struct sockaddr_in);
		rcvd = recvfrom(tro_sockfd, buf, MSS, 0, (struct sockaddr *)&troll_addr, &trolen);
		if (rcvd < 0){
			perror("Error receiving data from troll");
			close(loc_sockfd);
			close(tro_sockfd);
			exit(1);
		}
		if (rcvd < 16) {
			printf("Error! The packet size from troll is less than 16 bytes!\n");
			close(loc_sockfd);
			close(tro_sockfd);
			exit(1);
		}
		// Remove the first 16 bytes information in  troll packet which is about the sender
		bf = (char *)buf + 16; 
		// Then send them to ftps. The troll packet  may be truncated smaller if too large
		int left = rcvd - 16;
		ssize_t sent;
		size_t want_send = rqstlen;
		while (left > 0) {
			sent = sendto(loc_sockfd, bf, want_send, 0, (struct sockaddr *)&local_addr, socklen);
			if (sent < 0) {
				perror("Error sending data to ftps");
				close(loc_sockfd);
				close(tro_sockfd);
				exit(1);
			}
			left -= sent;
			bf = (char *)bf + sent;
			if (left > rqstlen) {
				want_send = rqstlen;
			} else {
				want_send = left;
			}
		} 
		free(buf);
		buf = NULL;
	}
}

/*
 * This functon reads all the bytes
 */
void recvAll(int sockfd, void *buf, int len, unsigned int flag, struct sockaddr *from, int *fromlen) {
	int bytesRead = 0;
	while(bytesRead < len) {
		fprintf(stderr, "Within recvAll.\n");
		bytesRead += recvfrom(sockfd, (void*)(((char*)buf)+bytesRead), len-bytesRead, flag, from, fromlen);
		fprintf(stderr, "Received %d bytes\n", bytesRead);
		fprintf(stderr, "Want %d bytes\n", len);
		// only useful for once
		if(bytesRead == -1) {
			fprintf(stderr, "tcpd -c: ERROR: Fail when receiving the file.\n");
			exit(-1);
		}
	}
	fprintf(stderr, "Exit recvAll\n");
}

/*
 * This functon sends all the bytes
 */
void sendAll(int sockfd, void *buf, int len, unsigned int flag, struct sockaddr *to, int tolen) {
	int bytesSent = 0;
	struct timespec timeOut, timeRm;
	timeOut.tv_sec = 0;
	timeOut.tv_nsec = 100000000;
	while(bytesSent < len) {
		bytesSent += sendto(sockfd, (void*)(((char*)buf)+bytesSent), len-bytesSent, flag, to, tolen);
		// only useful for once
		if(bytesSent == -1) {
			fprintf(stderr, "tcpd -c: ERROR: Fail when receiving the file.\n");
			exit(-1);
		}
		nanosleep(timeOut, timeRm);
	}
}

// Implementation of the client side of tcpd
void client() {
	int sockfdIn = -1;	/* socket for receiving data from ftpc */
    int sockfdOut = -1;	/* socket for sending data to troll */
    int headerSize = sizeof(struct sockaddr_in);	/* the size of the troll header */
	uint32_t fileSize = 0;	/* the size of the file */
	int64_t bytesLeft = 0;	/* the size of remaining content from ftpc */
	struct sockaddr_in inAddr;	/* the address of ftpc */
	struct sockaddr_in tcpdsAddr;	/* the address of server side tcpd */
    struct sockaddr_in trollAddr;	/* the address of troll */
	uint8_t *buf = malloc(sizeof(char) * MSS);
	socklen_t addrlen;
	struct hostent *host;
	char *servername = "eta.cse.ohio-state.edu";
	
	// create input socket
	// to receive data from ftpc
	sockfdIn = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfdIn == -1) {
		fprintf(stderr, "ERROR: tcpd can't create UDP socket.\n");
		exit(1);
	}
    // create output socket
	// to send data to troll
    sockfdOut = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfdOut == -1) {
		fprintf(stderr, "ERROR: tcpd can't create UDP socket.\n");
		close(sockfdIn);
		exit(1);
	}

	// bind input socket to port TCPD_PORT
	inAddr.sin_family = AF_INET;
	inAddr.sin_port = htons(1040);
	inAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(&(inAddr.sin_zero), '\0', 8);
	if(bind(sockfdIn, (struct sockaddr*)&inAddr, sizeof(inAddr)) != 0) {
		fprintf(stderr, "Error: bind.\n");
		exit(-1);
	}
    // set troll socket to port TROLL_IP and TROLL_PORT
	trollAddr.sin_family = AF_INET;
	trollAddr.sin_port = htons(1050);
	trollAddr.sin_addr.s_addr = inet_addr(TROLL_IP);
	memset(&(trollAddr.sin_zero), '\0', 8);
    // set server socket to port SERVER_IP and TROLL_PORT
	tcpdsAddr.sin_family = htons(AF_INET);
	tcpdsAddr.sin_port = htons(1050);
	host = gethostbyname(servername);
	if(host == NULL) {
		fprintf(stderr, "Bad address!\n");
		exit(-1);
	}
	//tcpdsAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	//memcpy((char*)&tcpdsAddr.sin_addr, host->h_addr, (host->h_length));
	bcopy(host->h_addr, (char*)&tcpdsAddr.sin_addr, host->h_length);
	memset(&(tcpdsAddr.sin_zero), '\0', 8);
    
	// read the first package and get the file size
	addrlen = sizeof(inAddr);
	recvfrom(sockfdIn, (void*)(buf+headerSize), MSS-headerSize, 0, (struct sockaddr*)&inAddr, &addrlen);
	// calculate the total size of data
	fileSize = *((uint32_t*)(buf+headerSize));
	bytesLeft = fileSize + 24l - (MSS + headerSize);
	fprintf(stderr, "%d bytes left\n", bytesLeft);
	// start to transfer
    while(1) {
        // add header
        memcpy(buf, (void*)&tcpdsAddr, sizeof(tcpdsAddr));
        // send to troll
        sendAll(sockfdOut, buf, MSS, 0, (struct sockaddr*)&trollAddr, sizeof(trollAddr));
        if(bytesLeft <= 0) {
            break;
        }
        int sizeRead = 0;
        // read from ftpc
        if(bytesLeft >= MSS - bufOffset) {
			sizeRead = MSS - bufOffset;
            recvAll(sockfdIn, (void*)(buf+bufOffset), MSS-bufOffset, 0, (struct sockaddr*)&inAddr, &addrlen);
        } else if(bytesLeft > 0) {
			sizeRead = bytesLeft;
            recvAll(sockfdIn, (void*)(buf+bufOffset), bytesLeft, 0, (struct sockaddr*)&inAddr, &addrlen);
        }
		bytesLeft -= sizeRead;
    }
}

void main(int argc, char *argv[]) {
	if (argc != 2) {
		printf("Error! The number of argument must be 2!\n");
		printf("Usage: tcpd -s to use on the server side.\n");
		printf("tcpd -c to use on the client side.\n");	
		exit(1);	
	}
	if (strcmp(argv[1], "-s") == 0) {
		server();
	} else if (strcmp(argv[1], "-c") == 0) {
		client();
	} else {
		printf("Error! The argument is not acceptable!\n");
		exit(1);
	}
}

