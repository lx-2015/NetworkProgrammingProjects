# CSE5462 Lab 2 Sp 2016
# Group member: Xiang Li, Haicheng Chen
# Makefile for ftps and ftpc

CC = gcc
OBJCLI = ftpc.c cse5462lib.c
OBJSRV = ftps.c cse5462lib.c
OBJTCP = tcpd.c
CFLAGS = -g
# setup for system
LIBS =

all: ftpc ftps tcpd

ftpc:	$(OBJCLI)
	$(CC) $(CFLAGS) -o $@ $(OBJCLI) $(LIBS)

ftps:	$(OBJSRV)
	$(CC) $(CFLAGS) -o $@ $(OBJSRV) $(LIBS)

tcpd:	$(OBJTCP)
	$(CC) $(CFLAGS) -o $@ $(OBJTCP) $(LIBS)

clean:
	rm -rf ftpc ftps tcpd FTPFolder
