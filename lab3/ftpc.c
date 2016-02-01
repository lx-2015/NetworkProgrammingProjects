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

#define BUF_SIZE 1000

/*
 * This function checks the validity of the port number
 */
int checkPort(char *port) {
	int i = 0;
	uint32_t sum = 0;
	// check length
	int length = strlen(port);
	if(length > 5) {
		return -1;
	}
	// check each digit and range
	for(i = 0; i < length; ++i) {
		if(port[i] < '0' || port[i] > '9') {
			return -1;
		} else {
			sum = (sum * 10) + (port[i] - '0');
			if(sum > 65536) {
				return -1;
			}
		}
	}

	return 0;
}

/*
 * This function checks the number of the arguments
 * as well as user's permission on the target file.
 * The connectability to the specified server will be checked later.
 */
int preCheckArgs(int argc, char *argv[]) {
	struct in_addr ipAddr;
	// check the number of arguments
	if(argc != 4) {
		fprintf(stderr, "ftpc: ERROR: Wrong usage of ftpc.\n");
		fprintf(stderr, "             Please use the following pattern to launch the ftpc program.\n");
		fprintf(stderr, "             $ ftpc <host-ip> <remote-port> <local-file-to-transfer>\n");
		return -1;
	}
	
	// check the IP address
	ipAddr.s_addr = inet_addr(argv[1]);
	if(strcmp(argv[1], "164.107.113.23") != 0) {	/* fake check */
		fprintf(stderr, "ftpc: ERROR: Invalid IP address %s. \n", argv[1]);
		return -1;
	}
	/*
	if(gethostbyaddr((char *)&ipAddr, sizeof(ipAddr), AF_INET)  == NULL) {
		fprintf(stderr, "ftpc: ERROR: Invalid IP address %s.\n", argv[1]);
		return -1;
	}
	*/

	// check the port
	if(checkPort(argv[2]) == -1) {
		fprintf(stderr, "ftpc: ERROR: Invalid port number %s.\n", argv[2]);
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
 */
int connectServer(int sockfd, char *ipAddress, char *port) {
	struct sockaddr_in serverAddress;
	struct hostent *host;
	
	// construct the server address
	serverAddress.sin_family = AF_INET;						/*sin_family*/
	serverAddress.sin_port = htons(1040);					/*sin_port*/
	serverAddress.sin_addr.s_addr= inet_addr("127.0.0.1");	/*sin_addr*/
	memset((void*)&(serverAddress.sin_zero), '\0', 8);		/*sin_zero*/

	// try to connect to the server
	if(CONNECT(sockfd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
		fprintf(stderr, "ftpc: ERROR: Can't connect to the server.\n");
		return -1;
	}

	return 0;
}

/*
 * This function guarantee to send all the content in the buf
 */
void sendAll(int sockfd, uint8_t *buf, int length) {
	int bytesSent = 0;
	while(bytesSent < length) {
		bytesSent += SEND(sockfd, buf+bytesSent, length-bytesSent, 0);
		if(bytesSent == -1) {
			fprintf(stderr, "ftpc: ERROR: Fail while sending the file.\n");
			exit(-1);
		}
	}
}

/*
 * This function updates the progress
 */
void updateProgress(uint32_t complete, uint32_t total) {
	int i = 0;
	unsigned percentage = (complete / (total / 100));
	printf("\rftpc: Sending file: %u%%", percentage);
	if(complete == total) {
		printf("\n");
	}
}

/*
 * This function sends the content in the target file
 * to the server trunk by trunk.
 */
void sendFileToServer(int sockfd, char *targetFile) {
	struct stat stat_buf;
	uint32_t fileSize = 0;
	uint8_t buf[BUF_SIZE];
	int bufOffset = 0;
	int fd = -1;
	uint32_t fileOffset = 0;

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
	memcpy((void*)buf, (void*)&htonl(fileSize), 4);	/* file size */
	bufOffset += 4;
	memcpy((void*)(buf+4), (void*)targetFile, strlen(targetFile));	/* file name */
	bufOffset += strlen(targetFile);
	buf[bufOffset] = '\0';
	bufOffset = 24;

	// fill the buffer
	printf("ftpc: Sending file: 0%%");
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
			return;
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
	sockfd = SOCKET(AF_INET, SOCK_STREAM, 0);
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

	return 0;
}
