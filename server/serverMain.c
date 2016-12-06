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
#include <pthread.h>
#include "queueList.h"
#include "serverUtils.h"
#include <signal.h>
#define BUFFSIZE 256
#define STATICDIRSIZE 1024
#define INT_DIGITS 19
#define MAX_CON 20


////////////////////////////////////////////Giving credit to///////////////////////////////////////////////
//stackoverflow.com/questions/4915538/recursing-directories-in-c?rq=1
//
//How to recurse through directories and print all of their contents
//=========================================================================================================
//www.opensource.apple.com/source/groff/groff-10/groff/libgroof/itoa.c
//
//itoa.c source code
//==========================================================================================================
//pubs.opengroup.org/onlinepubs/009695399/functions/pthread_sigmask.html
//
//Using signals with multithreaded processes to kill the program properly with Ctrl+C
//
//stackoverflow.com/questions/19509420/how-can-i-display-the-client-ip-address-on-the-server-side
//
//How to diplay the client ip address
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

Global_Utilities global_utilities;

int main(int argc,char* argv[]){

	char clientName[INET_ADDRSTRLEN];
	int newsock,i,rc,server_socket;
	long sz = sysconf(_SC_PAGESIZE);
	//long sz = 1;
	long ip;
	struct sockaddr_in server, client;
	struct sockaddr *serverptr = (struct sockaddr*) &server;
	struct sockaddr *clientptr = (struct sockaddr*) &client;
	struct hostent *rem;
	socklen_t clientlen;
	pthread_t producer;
	pthread_t sig_thr_id;
	ProducerArgs* serverArgs;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////ARGUMENT HANDLING////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	argumentHandling(argc,argv,&(global_utilities.port),&(global_utilities.queue_size),&(global_utilities.thread_pool_size));

	printf("MAIN THREAD: - Server's paramenters are:\n\tport: %d\n\tthread_pool_size: %d\n\tqueue_size: %d\n\n",global_utilities.port,global_utilities.thread_pool_size,global_utilities.queue_size);

	global_utilities.blocksize = sz; //BLOCKSIZE for file tranfers
	global_utilities.active_workers = 0; //Number of non-detached worker threads
	global_utilities.active_clients = 0; //Number of active clients currently
	global_utilities.accepting = 0; //Server accepting clients

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////CREATING A THREAD TO CATCH TERMINATION///////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	sigemptyset(&(global_utilities.signal_mask));
	sigaddset(&(global_utilities.signal_mask), SIGINT);
	sigaddset(&(global_utilities.signal_mask), SIGTERM);
	rc = pthread_sigmask(SIG_BLOCK, &(global_utilities.signal_mask), NULL);
	if(rc != 0){
		perror("MAIN THREAD - Signal mask installation");
		exit(1);
	}

	rc = pthread_create(&sig_thr_id, NULL, signal_thread, NULL);
	if(rc != 0){
		perror("MAIN THREAD - pthread_create for signals");
		exit(1);
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////CREATE MAIN QUEUE AND QUEUE OF CLIENT-ITEMS PAIRS////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	initQueue(&(global_utilities.queueOfItems),global_utilities.queue_size);
	initQueueP(&(global_utilities.queueOfPairs));

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////ALLOCATE SPACE FOR WORKER THREAD VARIABLES////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	printf("MAIN THREAD - Now we will allocate %d pointers for worker threads!\n",(global_utilities.thread_pool_size));
	global_utilities.worker_threads = NULL;
	global_utilities.worker_threads = malloc(sizeof(pthread_t)*(global_utilities.thread_pool_size));

	printf("MAIN THREAD - Allocated pointers for worker threads!\n");

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////INITIALISE SHARED MUTEXES AND CONDITION VARIABLES/////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	pthread_mutex_init(&(global_utilities.queue_mtx), NULL); //Queue access Mutex
	pthread_cond_init(&(global_utilities.cond_nonempty), NULL); //Condition queue == non empty
	pthread_cond_init(&(global_utilities.cond_nonfull), NULL); //Condition queue == non full
	pthread_mutex_init(&(global_utilities.readdir_mtx), NULL); //Readdir access Mutex
	pthread_mutex_init(&(global_utilities.malloc_mtx), NULL); //Malloc access Mutex
	pthread_mutex_init(&(global_utilities.free_mtx), NULL); //Free access Mutex
	//pthread_cond_init(&(global_utilities.cond_delivered), NULL); //Delivered an item condition (wakes a producer) 


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////REQUIRED NETWORKING CODE//////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	global_utilities.server_socket = 0;

	if((server_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0){ //Create a TCP Socket
		printf("MAIN THREAD - TCP socket creation failure! Destroying resources!\n");
		perror("socket");
		destroyQueue(&(global_utilities.queueOfItems));
		destroyQueueP(&(global_utilities.queueOfPairs));
		destroyWorkerResources();	
		exit(1);
	}

	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(global_utilities.port);
	server.sin_family = AF_INET; //internet addr family

	if(bind(server_socket,serverptr,sizeof(server)) < 0){
		printf("MAIN THREAD - Failed to BIND port: %d\n",global_utilities.port);
		perror("bind");
		destroyQueue(&(global_utilities.queueOfItems));
		destroyQueueP(&(global_utilities.queueOfPairs));
		destroyWorkerResources();
		//close(global_utilities.server_socket);
		exit(1);
	}

	printf("Listening for socket connection from client...\n");
	if(listen(server_socket,MAX_CON) < 0){ //listen for connections with Qsize == MAX_CON
		printf("MAIN THREAD - Failed to LISTEN on port: %d\n",global_utilities.port);
		perror("listen");
		close(server_socket);
		destroyQueue(&(global_utilities.queueOfItems));
		destroyQueueP(&(global_utilities.queueOfPairs));
		destroyWorkerResources();
		exit(1);
	}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////CREATE THE WORKER THREADS!/////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	for(i=0; i<(global_utilities.thread_pool_size); i++)
		pthread_create(&(global_utilities.worker_threads[i]), 0, Consumer, NULL); //GO MY WORKER THREADS!

	global_utilities.active_workers = global_utilities.thread_pool_size;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////BEGIN ACCEPTING CLIENTS/////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	global_utilities.accepting = 1;
	global_utilities.server_socket = server_socket;

	while(1){
		if(global_utilities.accepting == 1){
			if((newsock = accept(server_socket,clientptr,&clientlen)) < 0){ //Accept connection,ignore client address
				printf("MAIN THREAD - Accept failed! Server socket: |%d| and client socket: |%d|\n",global_utilities.server_socket,newsock);
				printf("accept - %s\n",strerror(errno));
				close(newsock);
			}
			else{
				//ip = ntohl(clientptr->sin_addr.s_addr);
				global_utilities.active_clients++;
				if(inet_ntop(AF_INET,&(client.sin_addr),clientName,sizeof(clientName)) == NULL) //Extract the IP
					printf("MAIN THREAD: - Unable to get IP address of new client!\n");
				printf("MAIN THREAD: - Accepted connection! New client with IP:|%s| on socket:|%d|\n",clientName,newsock);
				if(global_utilities.accepting == 1){ //To catch SIGINT right after accept check again in here
					pthread_mutex_lock(&(global_utilities.malloc_mtx));
					serverArgs = malloc(sizeof(ProducerArgs)); //Allocate the arguments of a new producer
					serverArgs->client_socket = newsock;
					serverArgs->server_socket = global_utilities.server_socket;
					pthread_create(&producer, 0, ServClCommunication, (void*)(serverArgs)); //Create a new producer
				}
				else{
					printf("MAIN THREAD - Unfortunately I won't be accepting any more clients!\n");
					if(write_all(newsock, "EXIT", 5) == -1){ //Tell client to exit
						printf("MAIN THREAD: - Telling client to leave: %s\n",strerror(errno));
					}
					close(newsock);
					global_utilities.active_clients--;
				}
			}
		}
	}
	
	return 0;	
}
