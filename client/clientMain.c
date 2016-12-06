#include <stdio.h>								
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>							//NOTE: SEE THE README INCLUDED TO BETTER UNDESTAND THE PROTOCOL
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <fcntl.h>
#define INT_DIGITS 19		/* enough for 64 bit integer */
#define STATICDIRSIZE 1024

void main(int argc, char* argv[]){

	int port,sock,i,optval,items,length;
	char buf[256];
	char original_dir[STATICDIRSIZE];
	char hostname[1024];
	char* numberbuf;
	char tmpbuf[20];
	char pathname[STATICDIRSIZE];
	char answerbuf[20];
	char* path;
	DIR *dip;
	struct dirent *dit;
	struct stat statbuf;
	struct sockaddr_in server;
	struct sockaddr *serverptr = (struct sockaddr*)&server;
	struct hostent *rem;

//////////////Argument Handling//////////////////////////
	argumentHandling(argc,argv,hostname,&port,pathname);

	printf("Client is running with arguments:\nServer IP: %s\nPort: %d\nDirectory: %s\n\n",hostname,port,pathname);

/////////////Required Networking code///////////////////
	if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) //Create socket
		perror_exit("socket");

	if((rem = gethostbyname(hostname)) == NULL){
		herror("gethostbyname");
		exit(1);
	}

	server.sin_family = AF_INET; //Internet domain
	memcpy(&server.sin_addr, rem->h_addr, rem->h_length);
	server.sin_port = htons(port);	//Server port
	
	printf("Attempting to connect...\n");

////////////Let's connect!/////////////////////////////
	if(connect(sock,serverptr,sizeof(server)) < 0)
		perror_exit("connect");
	printf("Connected to %s port %d!\n", hostname, port);

///////////Connected!//////////////////////////////////	
	printf("Going to request this directory:|%s|\n",pathname);

	if(getcwd(original_dir, sizeof(original_dir)) != NULL)
		printf("Current working dir: %s\n",original_dir);
	else{
		close(sock);
		perror_exit("getcwd() error");
	}

//////////The actual communication with the server///////
	if((ClServCommunication(sock,pathname,original_dir)) == -1)
		printf("Closing problematically...\n");

	printf("Closing the socket!\n");

//////////Waving goodbye to the server//////////////////	
	if(write_all(sock, "BYE", 4) == -1){
		close(sock);
		perror_exit("write1");
	}
	close(sock);

}
