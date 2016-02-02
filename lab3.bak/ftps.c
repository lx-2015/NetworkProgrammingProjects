// CSE5462 Sp2016 Lab3 
// Group member: Xiang Li, Haicheng Chen
// Date: 01/27/2016

// FTP server using UDP

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "cse5462lib.h"

#define BUF_SIZE 1000	// Buffer size
#define PORT 1060

// This function checks the validity of the port number
// But it is not used this time because the port number is hardcoded
int check_port(char *port) {
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

// Construct the connection. Note that *fsk is not used.
void construct_connection(int *sk, int *fsk, struct sockaddr_in *sia) {
	// Initialize socket connection in unix domain
	if((*sk = SOCKET(AF_INET, SOCK_DGRAM, 0)) < 0){
		perror("Error opening UDP socket");
		exit(1);
	}
  
	// Construct name of socket to send to
	sia->sin_family = AF_INET;
	sia->sin_addr.s_addr = htonl(INADDR_ANY);
	sia->sin_port = htons(PORT);
	memset(&(sia->sin_zero), '\0', 8);

	// Bind socket name to socket
	if(bind(*sk, (struct sockaddr *)sia, sizeof(struct sockaddr_in)) < 0) {
		perror("Error binding UDP socket");
		exit(1);
	}
}

// Helper function used to obtain the file information. 
// Update the file length *fln and return the filename
char* obtain_file_info(int file_sock, void *read_buf, size_t *rdct, int *fln) {
	// Keep reading until we have 24 bytes to process
	void *rdbf = read_buf;
	while (*rdct < 24) {
		/* read from file_sock and place in read_buf */
		ssize_t rc = RECV(file_sock, rdbf, BUF_SIZE - *rdct, 0);
		if (rc == -1) {
			perror("Error reading on stream socket");
			exit(1);
		}
		*rdct += rc;
		rdbf = (char*)rdbf + rc;
	}
	// Obtain the file length
	void *buf = malloc(4);
	memcpy(buf, read_buf, 4);
	// Change the host order	
	*fln = ntohl(*((int*)buf));
	free(buf);
	// Obtain the file name. Note that the file name is succeeded by a '\0'
	buf = malloc(20);
	// Shift the void* rdbf by 4 bytes, buf read_buf stays unchanged
	rdbf = read_buf;
	rdbf = (char*)rdbf + 4;
	memcpy(buf, rdbf, 20);
	char *buf1 = NULL;
	int fn_len;
	for (fn_len = 0; fn_len < 20; fn_len++) {
		buf1 = (char*)buf + fn_len;
		if (*((char*)buf1) == '\0') {
			break;
		}
	}
	char *filename = malloc((fn_len + 1) * sizeof(char));
	memccpy(filename, buf, '\0', 20);
	filename[fn_len] = '\0';
	return filename;
}

// Helper function used to make sure all bytes are indeed written
void write_all(int fd, void *buffer, size_t nbyte) {
	ssize_t wsz;
	size_t want = nbyte;
	while (want > 0) {
		wsz = write(fd, buffer, want);
		if (wsz == -1) {
			perror("Error writing the file");
			exit(1);
		}
		want -= wsz;
	}
}

// Write the file to local folder FTPFolder
void write_file(int file_sock, int fd, int file_len, void *read_buf, int read_count) {
	// Shift the void* rdbf by 20 bytes, buf read_buf stays unchanged
	void *rdbf = (char*)read_buf + 24;
	// Write the first portion of the file
	write_all(fd, rdbf, read_count - 24);
	// Zeros read_buf
	memset(read_buf, 0, read_count);
	int file_left_len = file_len;
	file_left_len -= read_count - 24;
	// Continue reading and writing
	while (file_left_len > 0) {
		read_count = RECV(file_sock, read_buf, BUF_SIZE, 0);
		if (read_count == -1) {
			perror("Error reading on stream socket");
			exit(1);
		}
		write_all(fd, read_buf, read_count);
		// Zeros read_buf
		memset(read_buf, 0, read_count);
		file_left_len -= read_count;
	}
}

// Server program called with no argument
int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("Number of argument is wrong! Usage is ftps 1060.\n");	
		exit(1);
	}
	char *port = argv[1];
	if (atoi(port) != 1060) {
		printf("Error! The port number must be 1060!\n");
	}
	/* This part is not used because the port number is hardcoded as 1060
	if (check_port(port) < 0) {
		printf("Error! The port number is invalid!\n");
		exit(1); 
	}
	*/
	
	int sock;                     // initial socket descriptor
	int file_sock;                /* accepted socket descriptor,
                                   * each client connection has a
                                   * unique socket descriptor.
								   * This variable is not used this time*/
	struct sockaddr_in sin_addr;  // structure for socket name setup

	construct_connection(&sock, &file_sock, &sin_addr);
	printf("FTP server waiting for remote connection from clients ...\n");
  	
	// Create a new folder and put the file in.
	char *dir = "FTPFolder";
	if (mkdir(dir, 0777) == -1) {
		if (errno != EEXIST) {
			perror("Error creating a new folder to hold the file");
			exit(1);
		} else {
			printf("Folder %s exists. Will put the file into this folder.\n", dir);
		}
	} else {
		printf("Create a directory named %s to hold the file.\n", dir);
	}

	// Obtain the file information
	// buffer for holding read data. Allocate memory and initialize to 0
	void *read_buf;               
	read_buf = calloc(BUF_SIZE, 1);
	size_t read_count = 0;
	int file_len = 0;
	int fn_len = 0;
	char *filename = obtain_file_info(file_sock, read_buf, &read_count, &file_len);
	printf("Server receiving file %s...\n", filename);
 
 	// Create the file with dir/filename to write and write the file
	char *full_filename = malloc(strlen(dir) + 1 + strlen(filename) + 1);
	strncpy(full_filename, dir, strlen(dir));
	full_filename[strlen(dir)] = '/';
	char *ffn = full_filename;
	ffn += strlen(dir) + 1;
	strncpy(ffn, filename, strlen(filename));
	ffn[strlen(filename)] = '\0';
	int fd = open(full_filename, O_CREAT | O_WRONLY | O_APPEND | O_TRUNC, 0777);
	if (fd == -1) {
		perror("Error creating the file");
		exit(1);
	}
	free(full_filename);
	full_filename = NULL;
	write_file(file_sock, fd, file_len, read_buf, read_count);
	
	// Clean the memory
	free(filename);
	filename = NULL;
	free(read_buf);
	read_buf = NULL;
	// close all connections and remove socket file
	close(fd);
	//close(file_sock);
	close(sock);
}
