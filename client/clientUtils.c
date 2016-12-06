#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "clientUtils.h"
#define INT_DIGITS 19		/* enough for 64 bit integer */
#define STATICDIRSIZE 1024

int sendOK(int sock){
	if(write_all(sock,"OK",3) == -1){
		perror("Failed to send OK");
		return -1;
	}
	return 0;
}

int getOK(int sock,char* buffer){
	if(read_all(sock,buffer,3) == -1){
		perror("Failed to receive OK");
		return -1;
	}
	return 0;
}

int argumentHandling(int argc,char** argv,char* hostname,int* port,char* pathname){ //Simply makes sure the arguments are correct in any possible order

	if(argc != 7){
		printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
		printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
		exit(1);
	}

	if(!strcmp(argv[1],"-p")){
		*port = atoi(argv[2]);
		if((*port >= 0) && (*port <= 99999)){
			if(!strcmp(argv[3],"-i")){
				strcpy(hostname,argv[3]);
				if(!strcmp(argv[5],"-d")){
					strcpy(pathname,argv[6]);
				}
				else{
					printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
					printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
					exit(1);
				}
			}
			else if(!strcmp(argv[3],"-d")){
				strcpy(pathname,argv[4]);
				if(!strcmp(argv[5],"-i")){
					strcpy(hostname,argv[6]);
				}
				else{
					printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
					printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
					exit(1);
				}
			}
			else{
				printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
				printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
				exit(1);
			}
		}
		else{
			printf("Invalid port number!\n");
			printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
			printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
			exit(1);
		}
	}
	else if(!strcmp(argv[1],"-i")){
		strcpy(hostname,argv[2]);
		if(!strcmp(argv[3],"-p")){
			*port = atoi(argv[4]);
			if((*port >= 0) && (*port <= 99999)){
				if(!strcmp(argv[5],"-d")){
					strcpy(pathname,argv[6]);
				}
				else{
					printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
					printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
					exit(1);
				}
			}
			else{
				printf("Invalid port number!\n");
				printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
				printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
				exit(1);
			}
		}
		else if(!strcmp(argv[3],"-d")){
				strcpy(pathname,argv[4]);
				if(!strcmp(argv[5],"-p")){
					*port = atoi(argv[6]);
					if((*port < 0) || (*port > 99999)){
						printf("Invalid port number!\n");
						printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
						printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
						exit(1);
					}
				}
				else{
					printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
					printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
					exit(1);
				}
		}
		else{
			printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
			printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
			exit(1);
		}
	}
	else if(!strcmp(argv[1],"-d")){
		strcpy(pathname,argv[2]);
		if(!strcmp(argv[3],"-p")){
			*port = atoi(argv[4]);
			if((*port >= 0) && (*port <= 99999)){
				if(!strcmp(argv[5],"-i")){
					strcpy(hostname,argv[6]);
				}
				else{
					printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
					printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
					exit(1);
				}
			}
			else{
				printf("Invalid port number!\n");
				printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
				printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
				exit(1);
			}
		}
		else if(!strcmp(argv[3],"-i")){
				strcpy(hostname,argv[4]);
				if(!strcmp(argv[5],"-p")){
					*port = atoi(argv[6]);
					if((*port < 0) || (*port > 99999)){
						printf("Invalid port number!\n");
						printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
						printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
						exit(1);
					}
				}
				else{
					printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
					printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
					exit(1);
				}
		}
		else{
			printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
			printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
			exit(1);
		}
	}
	else{
		printf("Client must be run like this: ./remoteClient -i <server_ip> -p <server_port> -d <directory>\n");
		printf("NOTE: Argument pairs (eg. -i <server_ip> is a pair) can be given in any order.\n");
		exit(1);
	}

}

int ClServCommunication(int sock,char* argument,char* original_dir){ //Main client-server communication

	int i,items,length;
	char buf[256];
	char* numberbuf;
	char tmpbuf[20];
	char answerbuf[20];
	char* path;

	//printf("Sending directory string size...\n");
	
	numberbuf = itoa(strlen(argument)+1);
	if(write_all(sock, numberbuf, INT_DIGITS+1) == -1){ //Sending directory length
		perror("write1");
		return -1;
	}

	getOK(sock,buf);

	if(!strcmp(buf,"OK")){
		//printf("Server successfully received the length of the pathname\n");
		printf("Sending to the server the pathname!\n");

		if(write_all(sock, argument, strlen(argument)+1) == -1){ //Sending directory pathname
			perror_exit("write2");
			return -1;
		}

		if((getOK(sock,buf)) == -1) return -1;

		if(!strcmp(buf,"OK")){

			if((sendOK(sock)) == -1) return -1;

			if((getOK(sock,buf)) == -1) return -1;

			if(!strcmp(buf,"OK")){
				//printf("Going to wait for this number of files to read!\n");

				if((sendOK(sock)) == -1) return -1;
		
				if(read_all(sock, tmpbuf, INT_DIGITS+1) == -1){ //Receive number of items to expect
					perror("read5");
					return -1;
				}
				items = atoi(tmpbuf);
				printf("I will receive %d items!\n",items);
		
				if((sendOK(sock)) == -1) return -1;
	
				if(items > 0){ //While not all items have been delivered
					i=0;
					while(items > 0){
						//printf("Receiving the length of the name of the %d file!\n",i+1);
						if(read_all(sock,tmpbuf,INT_DIGITS+1) == -1){ //Read length of pathname of file
							perror("read6");
							return -1;
						}
						length = atoi(tmpbuf);
	
						if((sendOK(sock)) == -1) return -1;
	
						path = malloc(sizeof(char)*(length));		
						//printf("Receiving the pathname of the %d file!\n",i+1);
						if(read_all(sock,path,length) == -1){ //Read pathname of file
							free(path);
							perror("read7");
							return -1;
						}
	
						if((createFileOnLocation(path,original_dir)) == -1){ //Create a file on the proper location in my system
							if(write_all(sock,"NOPE",3) == -1){
								free(path);
								perror("Failed to create file\n");
								return -1;
							}
						}
	
						if((sendOK(sock)) == -1){ 
							free(path); 
							return -1;
						}
					
						//printf("Beginning to receive file!\n");
						if(receiveFile(path,sock) == -1){ //Receive the file
							printf("File transfer failed at file:|%d|\n",i+1);
							free(path);
							perror("file_transfer");
							return -1;
						}
	
						if(read_all(sock,answerbuf,5) == -1){
							free(path);
							perror("NEXT");
							return -1;
						}
					
						if(!strcmp(answerbuf,"NEXT")){
							if((sendOK(sock)) == -1){ 
								free(path); 
								return -1;
							}
							//printf("Received successfully %s!\n",path);
							free(path);
						}
						else{
							free(path);
							perror("OK after NEXT");
							return -1;
						}
						i++;
						items--;	
					}
				}
				return 0;
			}
			else return -1;
		}
		else return -1;
	}
	else return -1;
}

int createFileOnLocation(char *path,char* originalDir){ //Creates a file copy of the one on the Server
														//If the pathname the client requested was absolute then the same path will be used here
														//If the pathname requested was relevant then a directory and subdirectories etc structure
														//will be created inside the current working directory of the client

	char pathname[STATICDIRSIZE];
	int countbackslash=0,i=0,j=0,k=0,increased=0,fd;
	char tmpbuffer[1024];
	struct stat statbuf;

	//printf("Server path of file: |%s|\n",path);

	if(path[0] != '/'){ //The pathname the client requested was RELEVANT
		//printf("Relative path given,must use the originalDir\n");
		strcpy(pathname,originalDir);
		strcat(pathname,"/");
		i=0;
		countbackslash=0;
		while(path[i] != '\0'){
			i++;
			if(path[i] == '/') countbackslash++;
		}
		if(countbackslash == 0){ //Number of backslashes in the path determines if we will create a file or a folder
			strcat(pathname,path);
			//printf("Simply create/open a file on location: |%s|!\n",pathname);
			if((fd = open(pathname,O_WRONLY)) == -1){ //Attemps to open the file/If it exists we will unlink and recreate it
													  //otherwise we will simply create it now and open it
			//perror("Could not open file,let's try to create it");

				if((fd = open(pathname,O_CREAT | O_WRONLY, 0777)) == -1){
					perror("Could not create file\n");
					return -1;
				}
				close(fd);
				return 1;

			}
			else{
				close(fd);
				if((unlink(pathname)) == -1){
					perror("Deleting file!\n");
					return -1;
				}
				if((fd = open(pathname,O_CREAT | O_WRONLY, 0777)) == -1){
					perror("Could not create file\n");
					return -1;
				}
				close(fd);
				return 1;
			}
		}
		k = strlen(pathname);
		j=0;
		i=0;
		//printf("Server path has %d backslashes!\n",countbackslash);
		while(j != countbackslash){ //Following the path and creating directories if needed,after the last backslash we will have reached the file name
			increased=0; //New backslash encountered flag
			while(increased == 0){
				pathname[k] = path[i];
				k++;
				pathname[k] = '\0'; //Always adding \0 after latest addition in order to print properly
				//printf("Adding letter to pathname,pathname: |%s|\n",pathname);
				if(path[i] == '/'){
					increased++; //New backslash,repeat process
					j++; //Increase backslash counter
		   			if(stat(pathname, &statbuf) == -1){ //Check if directory exists
		      			//perror("stat");
						//printf("Attempting to create directory |%s|\n",pathname);
						if(mkdir(pathname,0777) == -1){ //Create the directory then
							perror("mkdir");
						}	
		   			}
				}
				i++;
			}
		}
		while(path[i] != '\0'){
			pathname[k] = path[i];
			k++;
			i++;
		}
		pathname[k] = path[i]; //Completed pathname
		//printf("Relative-CASE2\n");
		//printf("Attempting to create file on location: |%s|\n",pathname);
		if((fd = open(pathname,O_WRONLY)) == -1){ //Same logic as in previous case,if it exists delete and reopen,if not create and open
			//perror("Could not open file,let's try to create it");

			if((fd = open(pathname,O_CREAT | O_WRONLY, 0777)) == -1){
				perror("Could not create file\n");
				return -1;
			}
			close(fd);
			return 1;

		}
		else{
			close(fd);
			if((unlink(pathname)) == -1){
				perror("Deleting file!\n");
				return -1;
			}
			if((fd = open(pathname,O_CREAT | O_WRONLY, 0777)) == -1){
				perror("Could not create file\n");
				return -1;
			}
			close(fd);
			return 1;
		}
	}
	else{ //Absolute path case
		//printf("Absolute path given,ignoring the originalDir\n");
		i=0;
		countbackslash=0;
		while(path[i] != '\0'){
			i++;
			if(path[i] == '/') countbackslash++;
		}
		i=0;
		j=0;
		k=0;
		while(j != countbackslash){
			increased=0;
			while(increased == 0){
				pathname[k] = path[i];
				k++;
				if(path[i] == '/'){
					increased++;
					j++;
					pathname[k] = '\0';
		   			if(stat(pathname, &statbuf) == -1){
		      			//perror("stat");
						//printf("Attempting to create directory |%s|\n",pathname);
						if(mkdir(pathname,0777) == -1){
							perror("mkdir");
						}	
		   			}
				}
				i++;
			}
		}
		while(path[i] != '\0'){
			pathname[k] = path[i];
			k++;
			i++;
		}
		pathname[k] = path[i];
		//printf("Relative-CASE2\n");
		//printf("Attempting to create file file on location: |%s|\n",pathname);
		if((fd = open(pathname,O_WRONLY)) == -1){
			//perror("Could not open file,let's try to create it");

			if((fd = open(pathname,O_CREAT | O_WRONLY, 0777)) == -1){
				perror("Could not create file\n");
				return -1;
			}
			close(fd);
			return 1;

		}
		else{
			close(fd);
			if((unlink(pathname)) == -1){
				perror("Deleting file!\n");
				return -1;
			}
			if((fd = open(pathname,O_CREAT | O_WRONLY, 0777)) == -1){
				perror("Could not create file\n");
				return -1;
			}
			close(fd);
			return 1;
		}
	}
}

void perror_exit(char* message){
	perror(message);
	exit(EXIT_FAILURE);
}

int write_all(int fd,void *buff,size_t size){ //Write() repeatedly until 'size' bytes are written!
	int sent, n;
	for(sent = 0; sent < size; sent+=n){
		if((n = write(fd, buff+sent, size-sent)) == -1)
			return -1;
	}
	return sent;
}

int read_all(int fd,void* buff,size_t size){

	int received, n;
	
	for(received = 0; received < size; received+=n){
		if((n = read(fd, buff+received, size-received)) == -1)
			return -1;
	}
	return received;
}

char *itoa(int i){
  /* Room for INT_DIGITS digits, - and '\0' */
  static char buf[INT_DIGITS + 2];
  char *p = buf + INT_DIGITS + 1;	/* points to terminating '\0' */
  if (i >= 0) {
    do {
      *--p = '0' + (i % 10);
      i /= 10;
    } while (i != 0);
    return p;
  }
  else {			/* i < 0 */
    do {
      *--p = '0' - (i % 10);
      i /= 10;
    } while (i != 0);
    *--p = '-';
  }
  return p;
}


int receiveFile(char* pathname,int sock){ //Function that receives the file

	int fd,filesize,n;
	char buf[1];
	char* tmpbuf;
	char answerbuf[20];
	char numberbuf[100];
	char* filebuffer;
	long blocksize;

	if((fd = open(pathname,O_WRONLY)) == -1){ //Open target
		perror("opening file to write on it");
		return -1;
	}
	
	//printf("Opened succesfully: %s\n",pathname);

	if(read_all(sock,answerbuf,INT_DIGITS+1) == -1){ //Get filesize
		perror("Receiving the file size from the server");
		close(fd);
		return -1;
	}

	filesize = atoi(answerbuf);
	//printf("Filesize as string: |%s|\n",answerbuf);
	if(filesize == -1){
		printf("Server failed to count filesize! Returning!\n");
		close(fd);
		return -1;
	}

	//printf("Received |%s|'s filesize: %d\n",pathname,filesize);

	sendOK(sock);

	if(read_all(sock,numberbuf,INT_DIGITS+1) == -1){ //Get blocksize the server uses
		perror("Receiving the block size\n");
		close(fd);
		return -1;
	}

	blocksize = atol(numberbuf);

	//printf("The blocksize the server uses is: %ld\n",blocksize);

	sendOK(sock);

	if(filesize == 0){
		printf("Received: |%s| succesfully!\n",pathname);
		close(fd);
		return 0;
	}
	
	while(filesize > 0){ //While not all of the file has been delivered
		if(filesize - blocksize > 0){ //Read blocksize size characters
			filesize = filesize - blocksize;
			filebuffer = malloc(sizeof(char)*blocksize);

			if(read_all(sock,filebuffer,blocksize) == -1){
				perror("error reading characters from socket");
				free(filebuffer);
				close(fd);
				return -1;
			}
	
			if(write_all(fd,filebuffer,blocksize) == -1){
				perror("error writing characters to file");
				free(filebuffer);
				close(fd);
				return -1;
			}

			free(filebuffer);
			filebuffer = NULL;
			sendOK(sock);
		}
		else{ //Less than blocksize characters left,use filesize
			filebuffer = malloc(sizeof(char)*filesize);

			if(read_all(sock,filebuffer,filesize) == -1){
				perror("error reading characters from socket");
				free(filebuffer);
				close(fd);
				return -1;
			}
	
			if(write_all(fd,filebuffer,filesize) == -1){
				perror("error writing characters to file");
				free(filebuffer);
				close(fd);
				return -1;
			}

			free(filebuffer);
			filebuffer = NULL;
			sendOK(sock);
			filesize = 0;
		}
	}
	printf("Received: |%s| succesfully!\n",pathname);
	close(fd);
	return 1;
}
