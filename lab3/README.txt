CSE5462 Sp2016 Lab3 README file
Group member: Xiang Li, Haicheng Chen
Date: 02/01/2016

1. All files needed to compile:
	Makefile: the makefile which will compile ftps, ftpc and tcpd all together.
	cse5462lib.h: header file of the capital letter socket related functions, e.g., SOCKET(), SEND(), etc.
	cse5462lib.c: implementation of the functions declared in cse5462lib.h
	tcpd.c: implementation of tcpd-server and tcpd-client
	ftps.c: implementation of the UDP server, where the original socket related functions
		are all replaced.
	ftpc.c: implementation of the UDP client, where the original socket related functions
		are all replaced.
		
2. How to make:
	Make sure the Makefile is already in the folder, and then type:

		% make

3. How to run:
	3.1 First the ftps and tcpd (server side) must be started from "eta.cse.ohio-state.edu"
		Start tcpd (server side) by typing the following command.
		This makes tcpd run at the background:

		% ./tcpd -s &
		
		Then start ftps by typing the following command. The port number must be 1060.
		
		% ./ftps 1060
		
		The program will block until a file is transferred.
	
	3.2 Then start troll on "gamma.cse.ohio-state.edu" by typing:
	
		% ./troll -b 1050 -x0
		
	3.3 Then start ftpc and tcpd (client side) from any stdlinux machine.
		First start tcpd (client side) by typing:
		
		% ./tcpd -c &
		
		Then start ftpc as follows and transfer a file. The IP address and port must be right.
		
		% ./ftpc 164.107.113.23 1060 MyFile
	
	After successfully send out the file, the client will automatically end itself. 
	Once the server successfully receives the file, it will automatically stop.
		
	