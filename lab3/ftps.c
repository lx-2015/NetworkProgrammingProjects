// CSE5462 Sp2016 Lab2 
// Group member: Xiang Li, Haicheng Chen
// Author of this file (ftps.c): Xiang Li
// Date: 01/21/2016

// FTP server using TCP

// Server for accepting an Internet stream connection on port 1040
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

#define PORT 1040       // socket file name
#define BUF_SIZE 1000	// Buffer size

// Construct the TCP connection
void construct_TCP(int *sk, int *fsk, struct sockaddr_in *sia) {
	// Initialize socket connection in unix domain
	if((*sk = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Error openting TCP socket!");
		exit(1);
	}
  
	// Construct name of socket to send to
	sia->sin_family = AF_INET;
	sia->sin_addr.s_addr = INADDR_ANY;
	sia->sin_port = htons(PORT);
	memset(&(sia->sin_zero), '\0', 8);

	// Bind socket name to socket
	if(bind(*sk, (struct sockaddr *)sia, sizeof(struct sockaddr_in)) < 0) {
		perror("Error binding stream socket");
		exit(1);
	}
  
	// Listen for socket connection and set max opened socket connetions to 1
	listen(*sk, 1);
  
	// Accept a (1) connection in socket msgsocket */ 
	if((*fsk = accept(*sk, (struct sockaddr *)NULL, (int *)NULL)) == -1) { 
		perror("Error connecting stream socket");
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
		ssize_t rc = recv(file_sock, rdbf, BUF_SIZE - *rdct, 0);
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
	*fln = *((int*)buf);
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
			perror("Error writing the file!");
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
		read_count = recv(file_sock, read_buf, BUF_SIZE, 0);
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
int main() {
	int sock;                     // initial socket descriptor
	int file_sock;                /* accepted socket descriptor,
                                   * each client connection has a
                                   * unique socket descriptor*/
	struct sockaddr_in sin_addr;  // structure for socket name setup

	printf("TCP server waiting for remote connection from clients ...\n");
	construct_TCP(&sock, &file_sock, &sin_addr);
  	
	// Create a new folder and put the file in.
	char *dir = "FTPFolder";
	if (mkdir(dir, 0777) == -1) {
		if (errno != EEXIST) {
			perror("Error creating a new folder to hold the file!");
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
		perror("Error creating the file!");
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
	close(file_sock);
	close(sock);
}
