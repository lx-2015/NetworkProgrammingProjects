#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define MIN_BUF_SIZE 1024;

/*
 * check the validity of the parameters
 */
void checkArgs(int argc, char *argv[]) {
	FILE *fpIn, *fpOut;
	struct stat stat_buf;
	// check argument number
	if(argc != 4) {
		printf("ERROR: Wrong usage of count.\n");
		printf("       Please use the following command to launch count:\n");
		printf("       $ ./count <input-filename> <search-string> <output-filename>\n");
		exit(-1);
	}
	// check input file
	if(access(argv[1], R_OK) == -1) {
		printf("ERROR: Input file %s cannot be read.\n", argv[1]);
		exit(-1);
	}
	// check output file
	if(stat(argv[3], &stat_buf) == 0) {
		printf("ERROR: Output file %s already exists.\n", argv[3]);
		exit(-1);
	}
}

/*
 * get the size of the input file
 */
void getFileSize(char *inputFileName, long *inputFileSize) {
	struct stat stat_buf;
	if(stat(inputFileName, &stat_buf) == 0) {
		*inputFileSize = stat_buf.st_size;
	} else {
		printf("ERROR: Can't stat input file.\n");
		exit(-1);
	}
}

/*
 * count the existence of the target file
 */
void findStrings(char *inputFileName, int inputFileSize, char *targetString, int *targetStringCount) {
	int fpIn = 0;
	int offset = 0;
	int bufSize = MIN_BUF_SIZE;
	int targetStringSize = strlen(targetString);
	char *buf = NULL;
	// define the size of read-in trunks
	if(targetStringSize >= bufSize) {
		bufSize = targetStringSize + 1;
	}
	buf = malloc(bufSize);
	// read contents from the file
	fpIn = open(inputFileName, O_RDONLY);
	if(fpIn == -1) {
		printf("ERROR: Can't open the input file\n");
		exit(-1);
	}
	while(offset < inputFileSize) {
		int readCount = pread(fpIn, buf, bufSize-1, offset);
		buf[readCount] = '\0';
		char *firstOccur;
		// stop if the remain content is shorter than the target string
		if(readCount < targetStringSize) {
			break;
		}
		// search for target string
		firstOccur = strstr(buf, targetString);
		if(firstOccur == NULL) {
			// find the first char if no match found
			firstOccur = index(buf+1, targetString[0]);
			// update the offset accordingly
			if(firstOccur == NULL) {
				offset += bufSize - 1;
			} else {
				offset += (firstOccur - buf);
			}
		} else {
			// increase target string count
			(*targetStringCount)++;
			// update offset
			firstOccur = index(firstOccur+1, targetString[0]);
			if(firstOccur == NULL) {
				offset += bufSize - 1;
			} else {
				offset += (firstOccur - buf);
			}
		}
	}
	close(fpIn);
}

/*
 * print out the result
 */
void printResult(long inputFileSize, int targetStringCount, char *outputFileName) {
	FILE *fpOut;
	fpOut = fopen(outputFileName, "w");
	fprintf(fpOut, "Size of file is %d\n", inputFileSize);
	fprintf(fpOut, "Number of matches = %d\n", targetStringCount);
	fclose(fpOut);
	printf("Size of file is %d\n", inputFileSize);
	printf("Number of matches = %d\n", targetStringCount);
}

/*
 * main function of the program
 */
int main(int argc, char *argv[]) {
	long inputFileSize = 0;
	int targetStringCount = 0;
	// check arguments
	checkArgs(argc, argv);
	// get input file size
	getFileSize(argv[1], &inputFileSize);
	// find target strings
	findStrings(argv[1], inputFileSize,  argv[2], &targetStringCount);
	// print the result
	printResult(inputFileSize, targetStringCount, argv[3]);

	return 0;
}
