/* CSE5462 Sp2016 Lab2
 * Group member: Xiang Li, Haicheng Chen
 * Author of this file (ftpc.c): Haicheng Chen
 * Last update: 01/25/2016
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "cse5462lib.h"

#define BUF_SIZE 984
#define SERVER_IP "164.107.113.23"
#define SERVER_PORT 1060
#define SERVER_PORT_STR "1060"

/*
 * This function checks the number of the arguments
 * as well as the validity of each argument.
 */
int preCheckArgs(int argc, char *argv[]) {
	// Check the number of arguments.
	if(argc != 4) {
		fprintf(stderr, "ftpc: ERROR: Wrong usage of ftpc.\n");
		fprintf(stderr, "             Please use the following pattern to launch the ftpc program.\n");
		fprintf(stderr, "             $ ftpc <host-ip> <remote-port> <local-file-to-transfer>\n");
		return -1;
	}
	
	// Check the IP address.
	// The user should provide the host IP address,
	// which is hardcoded in this project.
	if(strcmp(argv[1], SERVER_IP) != 0) {
		fprintf(stderr, "ftpc: ERROR: Please provide the server address %s. \n", SERVER_IP);
		return -1;
	}

	// Check the port.
	// The user should provide the host port number,
	// which is also hardcoded in this project.
	if(strcmp(argv[2], SERVER_PORT_STR) != 0) {
		fprintf(stderr, "ftpc: ERROR: Please provide the server port %s.\n", SERVER_PORT_STR);
		return -1;
	}

	// check the length of the file name
	if(strlen(argv[3]) > 20) {
		fprintf(stderr, "ftpc: ERROR: File name is longer than 20 Bytes.\n");
		return -1;
	}

	// check the permission of the local file
	if(access(argv[3], R_OK) == -1) {
		fprintf(stderr, "ftpc: ERROR: Can't open the target file %s to read.\n", argv[3]);
		return -1;
	}
}

/*
 * This function try to connect to ther server
 * with the IP address and port information
 * provided by the user.
 * In this project, this function does not have any effect.
 */
int connectServer(int sockfd, char *ipAddress, char *port) {
	struct sockaddr_in serverAddress;
	
	// construct the server address
	serverAddress.sin_family = AF_INET;						/*sin_family*/
	serverAddress.sin_port = htons((uint16_t)SERVER_PORT);	/*sin_port*/
	serverAddress.sin_addr.s_addr= inet_addr(SERVER_IP);	/*sin_addr*/
	memset((void*)&(serverAddress.sin_zero), '\0', 8);		/*sin_zero*/

	// try to connect to the server
	// In this project, this function always return 0.
	if(CONNECT(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
		fprintf(stderr, "ftpc: ERROR: Can't connect to the server.\n");
		return -1;
	}

	return 0;
}

/*
 * This function guarantee to send out all the content in the buf
 */
void sendAll(int sockfd, uint8_t *buf, int length) {
	int bytesSent = 0;
	struct timespec timeReq, timeRem;
	timeReq.tv_sec = 0;
	timeReq.tv_nsec = 10000000;
	while(bytesSent < length) {
		int sentInThisRun = 0;
		sentInThisRun = SEND(sockfd, (void*)(buf+bytesSent), length-bytesSent, 0);
		nanosleep(&timeReq, &timeRem);
		bytesSent += sentInThisRun;
		if(sentInThisRun == -1) {
			fprintf(stderr, "ftpc: ERROR: Fail while sending the file.\n");
			exit(-1);
		}
	}
}

/*
 * This function updates the progress
 */
void updateProgress(uint32_t complete, uint32_t total) {
	printf("ftpc: Bytes sent %u/%u.\n", complete, total);
}

/*
 * This function sends out the content in the target file
 * to the tcpd trunk by trunk.
 */
void sendFileToServer(int sockfd, char *targetFile) {
	struct stat stat_buf;
	uint32_t fileSize = 0;	/* size of the file */
	uint32_t fileOffset = 0;	/* size of the content which is sent */
	uint8_t buf[BUF_SIZE];	/* buf of the content to be sent */
	int bufOffset = 0;	/* records how many content in the file */
	int fd = -1;	/* the file descriptor for the sending file */

	// get the file size
	if(stat(targetFile, &stat_buf) == -1) {
		fprintf(stderr, "ftpc: ERROR: Can't get the status of the target file %s.\n", targetFile);
		exit(-1);
	}
	fileSize = stat_buf.st_size;
	
	// open the target file
	fd = open(targetFile, O_RDONLY);
	if(fd == -1) {
		fprintf(stderr, "ftpc: ERRORL Can't open file %s to read.\n", targetFile);
		exit(-1);
	}

	// create the header
	memcpy((void*)buf, (void*)&fileSize, 4);	/* file size */
	bufOffset += 4;	/* forward to the slot for file name */
	memcpy((void*)(buf+4), (void*)targetFile, strlen(targetFile));	/* file name */
	bufOffset += strlen(targetFile); /* forward to the end of the file name */
	buf[bufOffset] = '\0';	/* place '\0' to end the file name */
	bufOffset = 24;	/* forward to the slot for file content */

	// fill each buffer and send to tcpd
	while(1) {
		// read in the content
		int sizeRead = pread(fd, buf+bufOffset, BUF_SIZE - bufOffset, fileOffset);
		// update counters
		bufOffset += sizeRead;
		fileOffset += sizeRead;
		// send the buf if no more content in the file
		if(fileOffset >= fileSize) {
			sendAll(sockfd, buf, bufOffset);
			updateProgress(fileOffset, fileSize);
			break;
		}
		// send the buf if the buf is filled
		if(bufOffset == BUF_SIZE) {
			sendAll(sockfd, buf, bufOffset);
			updateProgress(fileOffset, fileSize);
			// reset data and update counters
			bufOffset = 0;
		}
	}

	// close the file
	close(fd);
}

/*
 * ftpc program sends the target file
 * to the server specified by the user
 * with ip address and port number.
 */
int main(int argc, char *argv[]) {
	int sockfd = -1;

	// precheck arguments
	if(preCheckArgs(argc, argv) == -1) {
		exit(-1);
	}

	// create the socket
	sockfd = SOCKET(AF_INET, SOCK_DGRAM, 0); // It's actually an UDP socket
	if(sockfd < 0) {
		fprintf(stderr, "ftpc: ERROR: Can't create the socket.\n");
		exit(-1);
	}

	// connect to the server
	if(connectServer(sockfd, /*ip address*/argv[1], /*port*/argv[2]) < 0) {
		exit(-1);
	}

	// send the file to the server
	sendFileToServer(sockfd, argv[3]);

	// close socket
	close(sockfd);

	return 0;
}
