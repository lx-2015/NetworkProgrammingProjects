// CSE5462 Sp2016 Lab3
// Group member: Xiang Li, Haicheng Chen
// Date: 01/27/2016

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

// This is the port tahtthat tcpd will always listen to
#define INNER_PORT 1040
#define LOCAL_IP "127.0.0.1"
#define MSS 1000

// Helper function used to make sure all data is sent
void send_all(nt sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
	ssize_t sent;
	size_t left = len;
	while (left > 0) {
		sent = sendto(sockfd, buf, len, flags, dest_addr, addrlen);
		if (sent == -1) {
			perror("Error sending packet to ftps!");
			exit(1);
		}
		left -= sent;
	}
}

// Helper function used to make sure a troll packet (size MSS) is obtained
void read_all(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
	void *bf = buf;
	size_t left_len = len;
	while (left_len > 0) {
		ssize_t rc = recvfrom(sockfd, bf, left_len, flags, src_addr, addrlen);
		if (rc == -1) {
			perror("Error reading from troll!");
			exit(1);
		}
		left_len -= rc;
		bf = (char *)bf + rc;
	}
}

// Implementation of the server side of tcpd
void server() {
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("Error opening UDP socket!");
		exit(1);	
	}	
	
	// First obtain the data from troll
	struct sockaddr_in troll_addr;
	troll_addr.sin_family = AF_INET;
	troll_addr.sin_port = htons(OUTER_PORT);
	troll_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(troll_addr.sin_zero), '\0', 8);	
	void *buf = malloc(sizeof(char) * MSS);
	read_all(sockfd, buf, MSS, 0, (sockaddr *)&troll_addr, sizeof(sockaddr_in *));

	// Remove the first 16 bytes information which is about troll
	buf = (char *)buf + 16; 
	
	// Then send them to ftps
	struct sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(INNER_PORT);
	local_addr.sin_addr.s_addr = inet_addr(LOCAL_IP);
	memset(&(local_addr.sin_zero), '\0', 8);	
	send_all(sockfd, buf, MSS - 16, 0, (struct sockaddr *)&local_addr, sizeof(sockaddr_in));
	free(buf);
	buf = NULL;
}

// Implementation of the client side of tcpd
void client() {
	int socketfdIn = -1;
    int socketfdOut = -1;
    int socketfdTroll = -1;
    int bufOffset = 0;
    int headerSize = sizeof(struct sockaddr_in);
	uint32_t fileSize = 0;
	uint64_t bytesLeft = 0;
	struct sockaddr_in inAddr;
	struct sockaddr_in outAddr;
    struct sockaddr_in trollAddr;
	void *buf = malloc(sizeof(char) * MSS);
	
	// create input socket
	socketfdIn = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd == -1) {
		fprintf(stderr, "ERROR: tcpd can't create UDP socket.\n");
		exit(1);
	}
    /*
	// create output socket
    socketfdOut = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd == -1) {
		fprintf(stderr, "ERROR: tcpd can't create UDP socket.\n");
		exit(1);
	}
	*/
    // create Troll socket
    socketfdTroll = socket(AF_INET, SOCK_DGRAM, 0);
	if(socketfd == -1) {
		fprintf(stderr, "ERROR: tcpd can't create UDP socket.\n");
		exit(1);
	}

	// bind input socket to port 1040
	inAddr.sin_family = AF_INET;
	inAddr.sin_port = htons(1040);
	inAddr.sin_addr.s_addr = inet_addr(LOCAL_IP);
	memset(&(inAddr.sin_zero), '\0', 8);
	bind(socketfdIn, (struct sockaddr*)&inAddr, sizeof(inAddr));
    // bind output socket to remote ip and port
	outAddr.sin_family = AF_INET;
	outAddr.sin_port = htons(1060);
	outAddr.sin_addr.s_addr = inet_addr("164.107.113.23");
	memset(&(outAddr.sin_zero), '\0', 8);
	/*bind(socketfdOut, (struct sockaddr*)&outAddr, sizeof(outAddr));*/
    // bind Troll socket to port 1050
	trollAddr.sin_family = AF_INET;
	trollAddr.sin_port = htons(1050);
	trollAddr.sin_addr.s_addr = inet_addr("164.107.113.20");
	memset(&(trollAddr.sin_zero), '\0', 8);
	bind(socketfdTroll, (struct sockaddr*)&trollAddr, sizeof(trollAddr));
    
	// get the file size
	recvfrom(socketfdIn, buf+headerSize, 4, 0, (struct sockaddr*)&inAddr, sizeof(inAddr));
	// calculate the total size of data
	fileSize = ntohl(*((int*)buf));
	bytesLeft = fileSize + 20l;
    bufOffset = 4 + headerSize;
	// start to transfer
    while(true) {
        int sizeRead = 0;
        // read from ftpc
        if(bytesLeft >= MSS - bufOffset) {
            sizeRead = recvfrom(socketfdIn, buf+bufOffset, MSS-bufOffset, 0, (struct sockaddr*)&inAddr, sizeof(inAddr));
        } else {
            sizeRead = recvfrom(socketfdIn, buf+bufOffset, bytesLeft, 0, (struct sockaddr*)&inAddr, sizeof(inAddr));
        }
        // add header
        memcpy(buf, (void*)&outAddr, sizeof(outAddr));
        // send to troll
        sendto(socketfdTroll, buf, MSS, 0, (struct sockaddr*)&trollAddr, sizeof(trollAddr));
        // update counter
        bufOffset = 4;
        bytesLeft -= sizeRead;
        if(bytesLeft == 0) {
            break;
        }
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
		printf("Error! The argument is not acceptable!");
		exit(1);
	}
}

