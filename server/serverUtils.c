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
#define STATICDIRSIZE 1024
#define INT_DIGITS 19
#define BUFFSIZE 256

extern Global_Utilities global_utilities;

char *name_from_address(struct in_addr addr){
	struct hostent *rem; int asize = sizeof(addr.s_addr);
	if((rem = gethostbyaddr(&addr.s_addr, asize, AF_INET)))
		return rem->h_name; //Reverse lookup success
	return inet_ntoa(addr);

}

int write_all(int fd,void *buff,size_t size){ //Write() repeatedly until 'size' bytes are written!
	int sent, n;
	for(sent = 0; sent < size; sent+=n){
		if((n = write(fd, buff+sent, size-sent)) == -1)
			return -1;
	}
	//if(!strcmp("18485",buff))
		//printf("Write sent: |%s| and sent |%d| bytes\n",(char*)buff,sent);
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

char *ltoa(long i){
  /* Room for INT_DIGITS digits, - and '\0' */
  static char buf[INT_DIGITS+INT_DIGITS + 2];
  char *p = buf + INT_DIGITS + +INT_DIGITS + 1;	/* points to terminating '\0' */
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

int sendOK(int newsock){
	if(write_all(newsock,"OK",3) == -1){
		//close(sock);
		//close(newsock);
		//if(dip != NULL) closedir(dip);
		//destroyQueue(&(global_utilities.queueOfItems));
		//destroyWorkerResources();
		printf("Failed to send OK: %s\n",strerror(errno));
		//pthread_detach(pthread_self());
		return -1;
	}
	return 0;
}

int getOK(int newsock,char* buffer){
	if(read_all(newsock,buffer,3) == -1){
		//close(sock);
		//close(newsock);
		//if(dip != NULL) closedir(dip);
		//destroyQueue(&(global_utilities.queueOfItems));
		//destroyWorkerResources();
		printf("Failed to receive OK: %s\n",strerror(errno));
		//pthread_detach(pthread_self());
		return -1;
	}
	return 0;
}

int argumentHandling(int argc,char** argv,int* port,int* queue_size,int* thread_pool_size){ //Simply makes sure the arguments are recognised in any order and are correct

	if(argc != 7){
		printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
		printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be give in any order.\n");
		exit(1);
	}
	if(!strcmp(argv[1],"-p")){
		*port = atoi(argv[2]);
		if((*port >= 0) && (*port <= 99999)){
			if(!strcmp(argv[3],"-s")){
				*thread_pool_size = atoi(argv[4]);
				if((*thread_pool_size > 0) && (*thread_pool_size <= THREAD_POOL_SIZE_MAX)){
					if(!strcmp(argv[5],"-q")){
						*queue_size = atoi(argv[6]);
						if(*queue_size < 0){
							printf("Invalid Queue size!\n");
							printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
							printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
							exit(1);	
						}
					}
					else{
						printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
						printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
						exit(1);	
					}
				}
				else{
					printf("Thread pool size max currently is: %d\n Please keep that in mind or redefine it!\n",THREAD_POOL_SIZE_MAX);
					printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
					printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
					exit(1);					
				}
			}
			else if(!strcmp(argv[3],"-q")){
				*queue_size = atoi(argv[4]);
				if(*queue_size > 0){
					if(!strcmp(argv[5],"-s")){
						*thread_pool_size = atoi(argv[6]);
						if((*thread_pool_size <= 0) || (*thread_pool_size > THREAD_POOL_SIZE_MAX)){
							printf("Thread pool size max currently is: %d\n Please keep that in mind or redefine it!\n",THREAD_POOL_SIZE_MAX);
							printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
							printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
							exit(1);						
						}
					}
					else{
						printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
						printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
						exit(1);
					}
				}
				else{
					printf("Invalid Queue size!\n");
					printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
					printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be give in any order.\n");
					exit(1);
				}
			}
			else{
				printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
				printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
				exit(1);
			}
		}
		else{
			printf("Invalid port number!\n");
			printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
			printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
			exit(1);
		}
	}
	else if(!strcmp(argv[1],"-s")){
		*thread_pool_size = atoi(argv[2]);
		if((*thread_pool_size > 0) && (*thread_pool_size <= THREAD_POOL_SIZE_MAX)){
			if(!strcmp(argv[3],"-p")){
				*port = atoi(argv[4]);
				if((*port >= 0) && (*port <= 99999)){
					if(!strcmp(argv[5],"-q")){
						*queue_size = atoi(argv[6]);
						if(*queue_size < 0){
							printf("Invalid Queue size!\n");
							printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
							printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
							exit(1);	
						}
					}
					else{
						printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
						printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
						exit(1);	
					}
				}
				else{
					printf("Invalid port number!\n");
					printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
					printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
					exit(1);					
				}
			}
			else if(!strcmp(argv[3],"-q")){
				*queue_size = atoi(argv[4]);
				if(*queue_size > 0){
					if(!strcmp(argv[5],"-p")){
						*port = atoi(argv[6]);
						if((*port < 0) || (*port >= 99999)){
							printf("Invalid port number!\n");
							printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
							printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
							exit(1);					
						}
					}
					else{
						printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
						printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
						exit(1);
					}
				}
				else{
					printf("Invalid Queue size!\n");
					printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
					printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
					exit(1);
				}
			}
			else{
				printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
				printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
				exit(1);
			}
		}
		else{
			printf("Thread pool size max currently is: %d\n Please keep that in mind or redefine it!\n",THREAD_POOL_SIZE_MAX);
			printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
			printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
			exit(1);
		}
	}
	else if(!strcmp(argv[1],"-q")){
		*queue_size = atoi(argv[2]);
		if(*queue_size > 0){
			if(!strcmp(argv[3],"-p")){
				*port = atoi(argv[4]);
				if((*port >= 0) && (*port <= 99999)){
					if(!strcmp(argv[5],"-s")){
						*thread_pool_size = atoi(argv[6]);
						if((*thread_pool_size <= 0) || (*thread_pool_size > THREAD_POOL_SIZE_MAX)){
							printf("Thread pool size max currently is: %d\n Please keep that in mind or redefine it!\n",THREAD_POOL_SIZE_MAX);
							printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
							printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
							exit(1);						
						}
					}
					else{
						printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
						printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
						exit(1);	
					}
				}
				else{
					printf("Invalid port number!\n");
					printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
					printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
					exit(1);					
				}
			}
			else if(!strcmp(argv[3],"-s")){
				*thread_pool_size = atoi(argv[4]);
				if((*thread_pool_size > 0) && (*thread_pool_size <= THREAD_POOL_SIZE_MAX)){
					if(!strcmp(argv[5],"-p")){
						*port = atoi(argv[6]);
						if((*port < 0) || (*port >= 99999)){
							printf("Invalid port number!\n");
							printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
							printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
							exit(1);					
						}
					}
					else{
						printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
						printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
						exit(1);
					}
				}
				else{
					printf("Thread pool size max currently is: %d\n Please keep that in mind or redefine it!\n",THREAD_POOL_SIZE_MAX);
					printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
					printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
					exit(1);	
				}
			}
			else{
				printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
				printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
				exit(1);
			}
		}
		else{
			printf("Invalid Queue size!\n");
			printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
			printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
			exit(1);
		}
	}
	else{
		printf("Server must be run like this: ./dataServer -p <port> -s <thread_pool_size> -q <queue_size>\n");
		printf("NOTE: Argument pairs (eg. -p <port> is a pair) can be given in any order.\n");
		exit(1);
	}

}

void* ServClCommunication(void* serverArgs){ //Tha main Producer function

	char cwd[STATICDIRSIZE];
	char accessingDir[STATICDIRSIZE];
	char numberbuf[20];
	char answerbuf[10];
	char* tmpword=NULL;
	char* tmpbuf=NULL;
	char* wordir;
	int port,length,optval,i,items_to_deliver,newsock,sock;
	int client_socket,server_socket;
	char buffer[BUFFSIZE];
	long sz = sysconf(_SC_PAGESIZE);
	DIR *dip;
	struct dirent *dit;
	struct stat statbuf;
	Queue tempQueue;
	pthread_mutex_t client_mtx;

	ProducerArgs* copy = (ProducerArgs*) serverArgs;

////////////////Get the arguments and the free the allocated memory for them////
	newsock = copy->client_socket;
	sock = copy->server_socket;

	pthread_mutex_lock(&(global_utilities.free_mtx));
	free(copy);
	copy = NULL;
	pthread_mutex_unlock(&(global_utilities.free_mtx));
	pthread_mutex_unlock(&(global_utilities.malloc_mtx));

	//printf("PRODUCER THREAD:|%lu| - I am a new producer,let's get to work!\n",pthread_self());

	pthread_mutex_init(&client_mtx,NULL); //Create a mutex for this specific client

	if(read_all(newsock, numberbuf, INT_DIGITS+1) == -1){ //Receive length of directory string
		if(write_all(newsock, "EXIT", 5) == -1){ //Tell client to exit
			printf("PRODUCER THREAD:|%lu| - write1: %s\n",pthread_self(),strerror(errno));
			pthread_mutex_destroy(&client_mtx);
			(global_utilities.active_clients)--;
			pthread_detach(pthread_self());
		}
		close(newsock);
		printf("PRODUCER THREAD:|%lu| - read1: %s\n",pthread_self(),strerror(errno));
		pthread_mutex_destroy(&client_mtx);
		(global_utilities.active_clients)--;
		pthread_detach(pthread_self());
	}

	//printf("Received successfully the length of the pathname!\n");

	if(sendOK(newsock) == -1){
		close(newsock);
		pthread_mutex_destroy(&client_mtx);
		(global_utilities.active_clients)--;
		pthread_detach(pthread_self());
	}

	length = atoi(numberbuf);

	if(read_all(newsock, accessingDir, length) == -1){ //Receiving directory
		if(write_all(newsock, "EXIT", 5) == -1){ //Tell client to exit
			printf("PRODUCER THREAD:|%lu| - write3: %s\n",pthread_self(),strerror(errno));
			pthread_mutex_destroy(&client_mtx);
			(global_utilities.active_clients)--;
			pthread_detach(pthread_self());
		}
		//close(sock);
		close(newsock);
		//destroyQueue(&(global_utilities.queueOfItems));
		//destroyWorkerResources();
		printf("PRODUCER THREAD:|%lu| - read2: %s\n",pthread_self(),strerror(errno));
		pthread_mutex_destroy(&client_mtx);
		(global_utilities.active_clients)--;
		pthread_detach(pthread_self());
	}

	if(sendOK(newsock) == -1){
		close(newsock);
		pthread_mutex_destroy(&client_mtx);
		(global_utilities.active_clients)--;
		pthread_detach(pthread_self());
	}

	printf("PRODUCER THREAD:|%lu| - About to scan directory - |%s|\n",pthread_self(),accessingDir);

	if(getOK(newsock,answerbuf) == -1){
		close(newsock);
		pthread_mutex_destroy(&client_mtx);
		(global_utilities.active_clients)--;
		pthread_detach(pthread_self());
	}

	if(!strcmp(answerbuf,"OK")){
	
		//Open Requested Directory
		if((dip = opendir(accessingDir)) == NULL){
			if(write_all(newsock,"EXIT",3) == -1){
				//close(sock);
				close(newsock);
				//destroyQueue(&(global_utilities.queueOfItems));
				//destroyWorkerResources();
				printf("PRODUCER THREAD:|%lu| - write7: %s\n",pthread_self(),strerror(errno));
				pthread_mutex_destroy(&client_mtx);
				(global_utilities.active_clients)--;
				pthread_detach(pthread_self());
			}
			//close(sock);
			close(newsock);
			//destroyQueue(&(global_utilities.queueOfItems));
			//destroyWorkerResources();
			printf("PRODUCER THREAD:|%lu| - opendir: %s\n",pthread_self(),strerror(errno));
			pthread_mutex_destroy(&client_mtx);
			(global_utilities.active_clients)--;
			pthread_detach(pthread_self());
		}

		if(sendOK(newsock) == -1){
			close(newsock);
			closedir(dip);
			pthread_mutex_destroy(&client_mtx);
			(global_utilities.active_clients)--;
			pthread_detach(pthread_self());
		}

		if(getOK(newsock,answerbuf) == -1){
			close(newsock);
			closedir(dip);
			pthread_mutex_destroy(&client_mtx);
			(global_utilities.active_clients)--;
			pthread_detach(pthread_self());
		}
	
		if(!strcmp(answerbuf,"OK")){
			//printf("Client is waiting for me to do work!\n");

			//printf("PRODUCER THREAD:|%lu| - Opened directory |%s| successfully!\n",pthread_self(),accessingDir);
			//Read everything in the directory
			//printf("Alright let's count the number of files!\n");		

			initQueue(&tempQueue,-1); //Create a tempQueue of unlimited length
			if((fillTheQueue(accessingDir,dip,newsock,&tempQueue,&client_mtx)) == -1){
				printf("PRODUCER THREAD:|%lu| - Failed to fill tempQueue: %s\n",pthread_self(),strerror(errno));
				//close(sock);
				close(newsock);
				//destroyQueue(&(global_utilities.queueOfItems));
				pthread_mutex_lock(&(global_utilities.free_mtx));
				destroyQueue(&tempQueue);
				pthread_mutex_unlock(&(global_utilities.free_mtx));
				//destroyWorkerResources();
				closedir(dip);
				pthread_mutex_destroy(&client_mtx);
				(global_utilities.active_clients)--;
				pthread_detach(pthread_self());
			}
			closedir(dip);

			//printf("Now let's send him the number of items to expect!\n");
			tmpbuf = itoa(tempQueue.items);
			if(write_all(newsock,tmpbuf,INT_DIGITS+1) == -1){ //Send the number of items to the client for him to expect
				//close(sock);
				printf("PRODUCER THREAD:|%lu| - write number of items to client: %s\n",pthread_self(),strerror(errno));
				close(newsock);
				pthread_mutex_lock(&(global_utilities.free_mtx));
				destroyQueue(&tempQueue);
				pthread_mutex_unlock(&(global_utilities.free_mtx));
				//destroyQueue(&(global_utilities.queueOfItems));
				//destroyWorkerResources();
				closedir(dip);
				pthread_mutex_destroy(&client_mtx);
				(global_utilities.active_clients)--;
				pthread_detach(pthread_self());
			}
	
			//printf("|PRODUCER|:Now let's see what the client will be able to receive!\n");
	
			//printQueue(tempQueue);

			if(getOK(newsock,answerbuf) == -1){
				close(newsock);
				pthread_mutex_lock(&(global_utilities.free_mtx));
				destroyQueue(&tempQueue);
				pthread_mutex_unlock(&(global_utilities.free_mtx));				
				pthread_mutex_destroy(&client_mtx);
				(global_utilities.active_clients)--;
				pthread_detach(pthread_self());
			}
				
			if(!strcmp(answerbuf,"OK")){
				printf("PRODUCER THREAD:|%lu| - Client has received the number of files!\n",pthread_self());
		
				pthread_mutex_lock(&(global_utilities.malloc_mtx));
				addQueueP(&(global_utilities.queueOfPairs),tempQueue.items,newsock); //Add to the queue of Pairs this pair of client mutex and items to deliver
				pthread_mutex_unlock(&(global_utilities.malloc_mtx));				 //Workers will reduce this number and also use this client mutex
				
				placeInMainQueue(&tempQueue); //Producer attemps to place one at a time an item from tempQueue to MainQueue

				pthread_mutex_lock(&(global_utilities.free_mtx));
				destroyQueue(&tempQueue); //Destroy tempQueue no longer needed
				pthread_mutex_unlock(&(global_utilities.free_mtx));

				//pthread_mutex_lock(&(client_mtx));


//////////////////////////////////////PRODUCER JOB IS DONE////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////WAIT FOR ALL MY ITEMS TO BE DELIVERED AND THEN DETACH///////////////////////////////////////////////////////
			
				items_to_deliver = getItemsOfClientQueueP(&(global_utilities.queueOfPairs),newsock); //Items to deliver for this producer's client
				while(items_to_deliver > 0){ //While not all the items delivered sleep for 1 second
					//printf("|PRODUCER|: The worker threads have not delivered everything yet...\n");
					sleep(1);
					//pthread_cond_wait(&(global_utilities.cond_delivered),&(client_mtx));
					items_to_deliver = getItemsOfClientQueueP(&(global_utilities.queueOfPairs),newsock);
				}
			
				i = getPositionOfClientQueueP(&(global_utilities.queueOfPairs),newsock);
				pthread_mutex_lock(&(global_utilities.free_mtx));
				removeElementOfQueueP(&(global_utilities.queueOfPairs),i);
				pthread_mutex_unlock(&(global_utilities.free_mtx));
				
				printf("PRODUCER THREAD:|%lu| - Seems like everything was delivered!\n",pthread_self());
				//printf("PRODUCER THREAD:|%lu| - Broadcasting to all worker threads to stop them!\n",pthread_self());
				//pthread_cond_broadcast(&(global_utilities.cond_nonempty));

				pthread_mutex_lock(&(client_mtx)); //Lock communication with my client just in case (useful for debugging in previous versions)
				if(read_all(newsock,answerbuf,4) == -1){
					//close(sock);
					printf("PRODUCER THREAD:|%lu| - Failed to wave GOODBYE with client: %s\n",pthread_self(),strerror(errno));
					close(newsock);
					//destroyQueue(&(global_utilities.queueOfItems));
					//destroyWorkerResources();
					pthread_mutex_unlock(&(client_mtx));
					pthread_mutex_destroy(&(client_mtx));
					(global_utilities.active_clients)--;
					pthread_detach(pthread_self());
				}
			
				if(!strcmp(answerbuf,"BYE")){ //Protocol has successfully been completed
					printf("PRODUCER THREAD:|%lu| - ALL OK, Client on socket: |%d| received everything properly!\n",pthread_self(),newsock);
					close(newsock);
					pthread_mutex_unlock(&(client_mtx));
					pthread_mutex_destroy(&(client_mtx));
					(global_utilities.active_clients)--;
					printf("PRODUCER THREAD:|%lu| - Other than me, %d producers run!!\n",pthread_self(),global_utilities.active_clients);
					printf("PRODUCER THREAD:|%lu| - JERONIMO!\n",pthread_self());
					pthread_detach(pthread_self());
				}
				else{
					printf("PRODUCER THREAD:|%lu| - Something went wrong!\n",pthread_self());
					close(newsock);
					pthread_mutex_unlock(&(client_mtx));
					pthread_mutex_destroy(&(client_mtx));
					(global_utilities.active_clients)--;
					pthread_detach(pthread_self());
				}

			}
			else{
				close(newsock);
				pthread_mutex_lock(&(global_utilities.free_mtx));
				destroyQueue(&tempQueue);
				pthread_mutex_unlock(&(global_utilities.free_mtx));				
				pthread_mutex_destroy(&client_mtx);
				(global_utilities.active_clients)--;
				pthread_detach(pthread_self());
			}
		}
		else{
			close(newsock);
			closedir(dip);
			pthread_mutex_destroy(&client_mtx);
			(global_utilities.active_clients)--;
			pthread_detach(pthread_self());
		}
	}
	else{
		close(newsock);
		pthread_mutex_destroy(&client_mtx);
		(global_utilities.active_clients)--;
		pthread_detach(pthread_self());
	}
}

int placeInMainQueue(Queue* tempQueue){ //Trasferring an item from the Temp Queue of its producer to the Main Queue of the program

	while(tempQueue->items > 0){ //While items left to transfer
		printf("PRODUCER THREAD:|%lu| - Adding file with pathname:|%s| to Queue...\n",pthread_self(),tempQueue->Head->pathname);
		pthread_mutex_lock(&(global_utilities.queue_mtx)); //Access the Main Queue

		while(global_utilities.queueOfItems.items == global_utilities.queueOfItems.max_size){ //Can't place anything in it
			//printf("PRODUCER THREAD:|%lu| - Main Queue is full,waiting...\n",pthread_self());
			pthread_cond_wait(&(global_utilities.cond_nonfull),&(global_utilities.queue_mtx)); //Wait for a consumer to wake me up
		}
		pthread_mutex_lock(&(global_utilities.malloc_mtx));
		addQueue(&(global_utilities.queueOfItems),tempQueue->Head->pathname,tempQueue->Head->client_socket,tempQueue->Head->client_mtx); //Place an item in the Main Queue
		pthread_mutex_unlock(&(global_utilities.malloc_mtx));

		//printf("PRODUCER THREAD:|%lu| - Added file with pathname:|%s| to Queue!\n",pthread_self(),tempQueue->Head->pathname);

		pthread_mutex_lock(&(global_utilities.free_mtx));
		removeElementOfQueue(tempQueue,1);
		pthread_mutex_unlock(&(global_utilities.free_mtx));
		
		//printf("PRODUCER THREAD:|%lu| - The Main Queue currently contains:|%d| items\n",pthread_self(),global_utilities.queueOfItems.items);
		pthread_mutex_unlock(&(global_utilities.queue_mtx));
		//printf("PRODUCER THREAD:|%lu| - Signaling worker threads to work!\n",pthread_self());
		pthread_cond_signal(&(global_utilities.cond_nonempty));
	}
}

int fillTheQueue(char* accessingDir,DIR* dip,int newsock,Queue* queueptr,pthread_mutex_t* client_mtx){ //Fills the tempQueue of its producer with the pathnames
																									   //of all the files in the given directory accessingDir and its
																									   //and its subdirectories

	struct dirent *dit;
	char* tmpword;

	do{
		pthread_mutex_lock(&(global_utilities.readdir_mtx)); //Make readdir threadsafe
		if((dit = readdir(dip)) == NULL){ //No more stuff in the directory
			pthread_mutex_unlock(&(global_utilities.readdir_mtx));
 			break;
		}
		pthread_mutex_unlock(&(global_utilities.readdir_mtx));
		if((!strcmp(dit->d_name, ".")) || (!strcmp(dit->d_name, ".."))){ //Ignore . and ..
			//printf("Skipping . and ..\n");
	    	continue;
		}

		if(dit->d_type == 8){ //FILE LOCATED
			//printf("File located! Its name is:|%s|\n",dit->d_name);
			tmpword = NULL;
		
			pthread_mutex_lock(&(global_utilities.malloc_mtx));
			tmpword = malloc(sizeof(char)*(strlen(accessingDir)+strlen(dit->d_name)+2));
			pthread_mutex_unlock(&(global_utilities.malloc_mtx));

			strcpy(tmpword,accessingDir); //Correcting the pathname for the file
			strcat(tmpword,"/");
			strcat(tmpword,dit->d_name);
			

			pthread_mutex_lock(&(global_utilities.malloc_mtx));
			addQueue(queueptr,tmpword,newsock,client_mtx); //Place the pathname into the Temp Queue
			pthread_mutex_unlock(&(global_utilities.malloc_mtx));

			//printf("PRODUCER THREAD:|%lu| - Added file with pathname:|%s| to tempQueue!\n",pthread_self(),tmpword);
	
			pthread_mutex_lock(&(global_utilities.free_mtx));
			free(tmpword);
			pthread_mutex_unlock(&(global_utilities.free_mtx));
		}
		else if(dit->d_type == 4){ //DIRECTORY LOCATED
			//printf("Directory located! Its name is:|%s|\n",dit->d_name);
			tmpword = NULL;
	
			pthread_mutex_lock(&(global_utilities.malloc_mtx));
			tmpword = malloc(sizeof(char)*(strlen(accessingDir)+strlen(dit->d_name)+2));
			pthread_mutex_unlock(&(global_utilities.malloc_mtx));

			strcpy(tmpword,accessingDir);
			strcat(tmpword,"/");
			strcat(tmpword,dit->d_name);
			checkNextDir(tmpword,newsock,queueptr,client_mtx); //Moving to the next directory recursively

			pthread_mutex_lock(&(global_utilities.free_mtx));
			free(tmpword);
			pthread_mutex_unlock(&(global_utilities.free_mtx));
		}
	}while(1);
	return 0;
}

int checkNextDir(char* accessingDir,int newsock,Queue* queueptr,pthread_mutex_t* client_mtx){ //Same as fillTheQueue,required for subsequent recursions
	
	char* tmpword=NULL;
	DIR* dip;
	struct dirent* dit;
	//printf("Attempting to open directory: |%s|\n",accessingDir);
	dip = opendir(accessingDir);
	//printf("Opened directory: |%s|\n",accessingDir);

	do{
		pthread_mutex_lock(&(global_utilities.readdir_mtx));
		if((dit = readdir(dip)) == NULL){
			pthread_mutex_unlock(&(global_utilities.readdir_mtx));
			break;
		}
		pthread_mutex_unlock(&(global_utilities.readdir_mtx));

		if((!strcmp(dit->d_name, ".")) || (!strcmp(dit->d_name, ".."))) continue;

		if(dit->d_type == 8){ //Could also use d_type 8
			tmpword = NULL;

			pthread_mutex_lock(&(global_utilities.malloc_mtx));
			tmpword = malloc(sizeof(char)*(strlen(accessingDir)+strlen(dit->d_name)+2));
			pthread_mutex_unlock(&(global_utilities.malloc_mtx));

			strcpy(tmpword,accessingDir);
			strcat(tmpword,"/");
			strcat(tmpword,dit->d_name);


			pthread_mutex_lock(&(global_utilities.malloc_mtx));
			addQueue(queueptr,tmpword,newsock,client_mtx);
			pthread_mutex_unlock(&(global_utilities.malloc_mtx));

			//printf("PRODUCER THREAD:|%lu| - Added file with pathname:|%s| to Queue!\n",pthread_self(),tmpword);

			pthread_mutex_lock(&(global_utilities.free_mtx));
			free(tmpword);
			pthread_mutex_unlock(&(global_utilities.free_mtx));
		}
		else if(dit->d_type == 4){ //Could also use d_type 4
			tmpword = NULL;

			pthread_mutex_lock(&(global_utilities.malloc_mtx));
			tmpword = malloc(sizeof(char)*(strlen(accessingDir)+strlen(dit->d_name)+2));
			pthread_mutex_unlock(&(global_utilities.malloc_mtx));

			strcpy(tmpword,accessingDir);
			strcat(tmpword,"/");
			strcat(tmpword,dit->d_name);
			checkNextDir(tmpword,newsock,queueptr,client_mtx);

			pthread_mutex_lock(&(global_utilities.free_mtx));
			free(tmpword);
			pthread_mutex_unlock(&(global_utilities.free_mtx));
		}
	}while(1);
	closedir(dip);
}

void perror_exit(char *message){
	perror(message);
	exit(EXIT_FAILURE);
}

void* Consumer(void* argsptr){ //Worker Thread main function,attempts to take an item from the Main Queue. Blocks when Main Queue is empty.
							   //Detaches itself when Main Queue has been destroyed. Each time it delivers (or fails) to deliver an item, it alters
							   //the items_to_deliver of the producer associated with the client_socket the consumer delivers, allowing the producer to detach

	char pathname[1024];
	int client_socket,items,server_socket,rc;
	long block_size;
	pthread_mutex_t* client_mtx;

	server_socket = global_utilities.server_socket;
	block_size = global_utilities.blocksize;


	do{
		pthread_mutex_lock(&(global_utilities.queue_mtx)); //Locking the queue mutex
		//printf("WORKER THREAD:|%lu| - Accessing the Queue...\n",pthread_self());
		while((global_utilities.queueOfItems.items == 0) && (global_utilities.queueOfItems.working == 1)){ //While the Queue is existing and no items are loaded on it
			//printf("WORKER THREAD:|%lu| - Queue is empty, waiting...\n",pthread_self());
			if(global_utilities.queueOfItems.working == 1)
				pthread_cond_wait(&(global_utilities.cond_nonempty),&(global_utilities.queue_mtx)); //Wait until signaled that an item has been loaded
			else break;
		}
		if(global_utilities.queueOfItems.working == 0){ //Time to detach this worker thread,Main Queue is destroyed
			//printf("WORKER THREAD:|%lu| - Nothing left to deliver.\n",pthread_self());
			pthread_mutex_unlock(&(global_utilities.queue_mtx));
			break;
		}
		//printf("WORKER THREAD:|%lu| - Let's get to work!!!\n",pthread_self());
		strcpy(pathname,global_utilities.queueOfItems.Head->pathname); //Extract the pathname
		client_socket = global_utilities.queueOfItems.Head->client_socket; //Extract this client's socket
		client_mtx = global_utilities.queueOfItems.Head->client_mtx; //Extract this client's mutex
	
		pthread_mutex_lock(&(global_utilities.free_mtx));
		removeElementOfQueue(&(global_utilities.queueOfItems),1); //Remove an element from the queue
		pthread_mutex_unlock(&(global_utilities.free_mtx));
		
		pthread_mutex_unlock(&(global_utilities.queue_mtx)); //Unlock queue mutex

		pthread_cond_signal(&(global_utilities.cond_nonfull)); //Signal that the queue is not full anymore to the producers

		printf("WORKER THREAD:|%lu| - Received task: |%s|\n",pthread_self(),pathname);

		pthread_mutex_lock(client_mtx); //Locking the communication with this client
		//printf("WORKER THREAD:|%lu| - Let's transfer |%s|!\n",pthread_self(),pathname);
		//printf("WORKER THREAD:|%lu| - Client socket is |%d|!\n",pthread_self(),client_socket);

//////////////////////////Consumer attempts to deliver item,in case of failure producer will know to detach///////////
/////////////////////////in case of success the items_to_deliver of this client_socket will be reduced by one/////////

		rc = 0;
		if((rc = Transfer(client_socket,pathname,block_size)) == -1){
			//printf("WORKER THREAD:|%lu| - Telling the Producer to stop serving the client since Transfer FAILED!\n",pthread_self());
			items=0;	//Transfer failed
			setItemsOfClientQueueP(&(global_utilities.queueOfPairs),client_socket,items); //Set this client's items_to_deliver 0 for producer to detach
		}
		else{//printf("WORKER THREAD:|%lu| - Delivered!!\n",pthread_self());	
			items=0;
			items = getItemsOfClientQueueP(&(global_utilities.queueOfPairs),client_socket); //Reduce this client's items by one
			if(items != -1){
				items--;
				if(items >= 0)
					setItemsOfClientQueueP(&(global_utilities.queueOfPairs),client_socket,items);
			}
		}
		pthread_mutex_unlock(client_mtx); //Allowing other threads to server this client

		//pthread_cond_broadcast(&(global_utilities.cond_delivered)); //Broadcast to any waiting producers that a delivery has taken place
	}while(1);

	printf("WORKER THREAD:|%lu| - BYE-BYE...\n",pthread_self());
	(global_utilities.active_workers)--;
	pthread_detach(pthread_self());
}

int Transfer(int newsock,char* pathname,long blocksize){ //Beginning file transfer

	char* tmpbuf;
	char answerbuf[20];

	printf("WORKER THREAD:|%lu| - About to read |%s|!\n",pthread_self(),pathname);
	tmpbuf = itoa(strlen(pathname)+1);
	if(write_all(newsock,tmpbuf,INT_DIGITS+1) == -1){ //Send file pathname length
		//close(sock);
		//close(newsock);
		//destroyQueue(&(global_utilities.queueOfItems));
		//destroyWorkerResources();
		printf("WORKER THREAD:|%lu| - Failed to send pathname length to client: %s\n",pthread_self(),strerror(errno));
		//pthread_exit(0);
		return -1;
	}
			
	if(getOK(newsock,answerbuf) == -1) return -1;

	if(!strcmp(answerbuf,"OK")){					

		if(write_all(newsock,pathname,strlen(pathname)+1) == -1){ //Send pathname
			//close(sock);
			//close(newsock);
			//destroyQueue(&(global_utilities.queueOfItems));
			//destroyWorkerResources();
			printf("WORKER THREAD:|%lu| - Failed to send pathname to client: %s\n",pthread_self(),strerror(errno));
			//pthread_exit(0);
			return -1;
		}

		if(getOK(newsock,answerbuf) == -1) return -1;

		if(!strcmp(answerbuf,"OK")){
			//printf("Beginning to send the file as blocks!\n");
			if(sendFile(pathname,newsock,blocksize) == -1){ //Actually send the file
				//printf("File transfer failed at file:|%d|\n",i+1);
				//destroyQueue(&(global_utilities.queueOfItems));
				//close(sock);
				//close(newsock);
				//destroyQueue(&(global_utilities.queueOfItems));
				//destroyWorkerResources();
				//closedir(dip);
				printf("WORKER THREAD:|%lu| - Failed to transfer file to client: %s\n",pthread_self(),strerror(errno));
				//pthread_exit(0);
				return -1;
			}
							
			if(write_all(newsock,"NEXT",5) == -1){ //Move successfully to the next file transfer for this client
				//close(sock);
				//close(newsock);
				//closedir(dip);
				//destroyQueue(&(global_utilities.queueOfItems));
				//destroyWorkerResources();
				printf("WORKER THREAD:|%lu| - Failed to make sure the client is ready for more files: %s\n",pthread_self(),strerror(errno));
				return -1;
				//pthread_exit(0);
			}
							
			if(getOK(newsock,answerbuf) == -1) return -1;
								
			if(strcmp(answerbuf,"OK")){ //Not ready for the next file transfer,file not transferred successfully
				//close(sock);
				//close(newsock);
				//closedir(dip);
				//destroyQueue(&(global_utilities.queueOfItems));
				//destroyWorkerResources();
				printf("WORKER THREAD:|%lu| - Not moving to next file!: %s\n",pthread_self(),strerror(errno));
				//pthread_exit(0);
				return -1;
			}
	
			printf("WORKER THREAD:|%lu| - Transferred |%s| successfully!\n",pthread_self(),pathname);
			
			return 0;
		}
		else return -1;
	}
	else return -1;
}

int sendFile(char* pathname,int newsock,long block_size){ //The actual function that send a file

	int fd,filesize,n,bla;
	char buf[1];
	char* tmpbuf;
	char* tmpbuf2;
	char answerbuf[20];
	char* filebuffer;

	if((fd = open(pathname,O_RDONLY)) == -1){ //Opening the specified file
		printf("WORKER THREAD:|%lu| - Opening: |%s| - FAILED: %s\n",pthread_self(),pathname,strerror(errno));
		tmpbuf = itoa(-1);
		if(write_all(newsock,tmpbuf,sizeof(int)) == -1){
			printf("WORKER THREAD:|%lu| - Failed to inform client about not opening file - %s\n",pthread_self(),strerror(errno));
		}
		return -1;
	}
	
	//printf("Opened succesfully: %s\n",pathname);

	filesize=0;
	while((n = read(fd,buf,1)) > 0) filesize++; //Counting its filesize
	if(n == -1){
		printf("WORKER THREAD:|%lu| -  Reading file size - %s\n",pthread_self(),strerror(errno));
		tmpbuf = itoa(-1);
		if(write_all(newsock,tmpbuf,sizeof(int)) == -1){
			printf("WORKER THREAD:|%lu| -  Failed to inform client about not counting filesize - %s\n",pthread_self(),strerror(errno));
		}
		return -1;
	}

	//printf("The size of the file  is: %d characters\n",filesize);
	
	close(fd);

	//printf("Sending the size of the file to the client\n");

	tmpbuf = itoa(filesize);
	//printf("Filesize as string:|%s|\n",tmpbuf);
	if(write_all(newsock,tmpbuf,INT_DIGITS+1) == -1){ //Sending the filesize
		printf("WORKER THREAD:|%lu| -  Sending the file size to the client - %s\n",pthread_self(),strerror(errno));
		return -1;
	}

	if(read_all(newsock,answerbuf,3) == -1){
		printf("WORKER THREAD:|%lu| -  Error receiving OK after filesize - %s\n",pthread_self(),strerror(errno));
		return -1;
	}

	if(!strcmp(answerbuf,"OK")){
		//printf("Client received the filesize\n");
		//printf("Now I will send him the block size!\n");
		
		//printf("The block size I will send is %ld\n",block_size);
		//tmpbuf = itoa(block_size);
		tmpbuf2 = ltoa(block_size);

		if(write_all(newsock,tmpbuf2,INT_DIGITS+1) == -1){ //Sending the blocksize
			printf("WORKER THREAD:|%lu| -  Sending the block size - %s\n",pthread_self(),strerror(errno));
			return -1;
		}

		//printf("I sent him |%s|\n",tmpbuf2);

		if(read_all(newsock,answerbuf,3) == -1){
			printf("WORKER THREAD:|%lu| -  Error receiving OK after blocksize - %s\n",pthread_self(),strerror(errno));
			return -1;
		}

		if(filesize == 0) return 0;

		if(!strcmp(answerbuf, "OK")){
			if((fd = open(pathname,O_RDONLY)) == -1){ //Reopening file
				printf("WORKER THREAD:|%lu| -  opening file2 - %s\n",pthread_self(),strerror(errno));
				return -1;
			}
		
			if(!strcmp(answerbuf,"OK")){
	
				while(filesize > 0){ //Begin sending the file one block at a time. If less space than a block is left,then send exactly the size left
					if(filesize - block_size > 0){
						filesize = filesize - block_size;

						pthread_mutex_lock(&(global_utilities.malloc_mtx));
						filebuffer = malloc(sizeof(char)*block_size);
						pthread_mutex_unlock(&(global_utilities.malloc_mtx));

						if(read_all(fd,filebuffer,block_size) == -1){ //Read characters for file
							printf("WORKER THREAD:|%lu| -  error reading characters from file - %s\n",pthread_self(),strerror(errno));
							free(filebuffer);
							close(fd);
							return -1;
						}
						if(write_all(newsock,filebuffer,block_size) == -1){ //Send them
							printf("WORKER THREAD:|%lu| -  error writing characters from file - %s\n",pthread_self(),strerror(errno));
							free(filebuffer);
							close(fd);
							return -1;
						}

						pthread_mutex_lock(&(global_utilities.free_mtx));
						free(filebuffer);
						pthread_mutex_unlock(&(global_utilities.free_mtx));

						filebuffer = NULL;
						if(read_all(newsock,answerbuf,3) == -1){
							printf("WORKER THREAD:|%lu| -  error receiving OK from client on block sending - %s\n",pthread_self(),strerror(errno));
							close(fd);
							return -1;
						}
					}
					else{

						pthread_mutex_lock(&(global_utilities.malloc_mtx));
						filebuffer = malloc(sizeof(char)*filesize);
						pthread_mutex_unlock(&(global_utilities.malloc_mtx));

						if(read_all(fd,filebuffer,filesize) == -1){
							printf("WORKER THREAD:|%lu| -  error reading characters from file2 - %s\n",pthread_self(),strerror(errno));
							free(filebuffer);
							close(fd);
							return -1;
						}
						if(write_all(newsock,filebuffer,filesize) == -1){
							printf("WORKER THREAD:|%lu| -  error writing characters from file2 - %s\n",pthread_self(),strerror(errno));
							free(filebuffer);
							close(fd);
							return -1;
						}

						pthread_mutex_lock(&(global_utilities.free_mtx));
						free(filebuffer);
						pthread_mutex_unlock(&(global_utilities.free_mtx));

						filebuffer = NULL;
						if(read_all(newsock,answerbuf,3) == -1){
							printf("WORKER THREAD:|%lu| -  error receiving OK from client on block sending2 - %s\n",pthread_self(),strerror(errno));
							close(fd);
							return -1;
						}
						filesize = 0;
					}
				}
				//printf("WORKER THREAD:|%lu| - I finished sending the file!\n",pthread_self());
				close(fd);
				return 1;
			}
		}	
		else{ 
			close(fd); 
			return -1;
		}
	}
	else return -1;
}

void destroyWorkerResources(){ //Destroys shared mutexes and condition variables
		int flag=0,i=0;		   //Does not destroy the client mutex a producer creates

		if(global_utilities.worker_threads != NULL){
			printf("DESTROYER: - Waiting for all worker threads to terminate...\n");
			while(global_utilities.active_workers > 0){ //Basically waits until all workers detach themselves. A worker detaches when he realises the Main queue has
														//been destroyed. The broadcast is required in order to wake them up if they are waiting for the Main queue to fill
														//When they wake up they will see that the Main Queue does not exist and then they will detach
				pthread_cond_broadcast(&(global_utilities.cond_nonempty));
				sleep(1);
			}
			
			pthread_mutex_lock(&(global_utilities.free_mtx));
			free(global_utilities.worker_threads);
			global_utilities.worker_threads = NULL;
			pthread_mutex_unlock(&(global_utilities.free_mtx));			

		}

		printf("DESTROYER: - Destroying condition variables...\n");
		flag=0;
		while((pthread_cond_destroy(&(global_utilities.cond_nonempty))) == EBUSY){
			if(flag == 0){
				printf("Condition Variable: cond_nonempty is locked and can't be destroyed yet!\n");
				flag = 1;
			}
			continue;
		}
		/*flag=0;
		while((pthread_cond_destroy(&(global_utilities.cond_delivered))) == EBUSY){
			if(flag == 0){
				printf("Condition Variable: cond_delivered is locked and can't be destroyed yet!\n");
				flag = 1;
			}
			continue;
		}*/
		flag=0;
		while((pthread_cond_destroy(&(global_utilities.cond_nonfull))) == EBUSY){
			if(flag == 0){
				printf("Condition Variable: cond_nonfull is locked and can't be destroyed yet!\n");
				flag = 1;
			}
			continue;
		}
		printf("Destroying worker mutexes...\n");
		flag=0;
		while((pthread_mutex_destroy(&(global_utilities.queue_mtx))) == EBUSY) { 
			if(flag == 0){
				printf("Queue Mutex is locked and can't be destroyed yet!\n");
				flag = 1;
			} 
			continue;
		}
		flag=0;
		while((pthread_mutex_destroy(&(global_utilities.readdir_mtx))) == EBUSY) { 
			if(flag == 0){
				printf("Readdir Mutex is locked and can't be destroyed yet!\n");
				flag = 1;
			} 
			continue;
		}
		flag=0;
		while((pthread_mutex_destroy(&(global_utilities.malloc_mtx))) == EBUSY) { 
			if(flag == 0){
				printf("Malloc Mutex is locked and can't be destroyed yet!\n");
				flag = 1;
			} 
			continue;
		}
		flag=0;
		while((pthread_mutex_destroy(&(global_utilities.free_mtx))) == EBUSY) { 
			if(flag == 0){
				printf("Free Mutex is locked and can't be destroyed yet!\n");
				flag = 1;
			} 
			continue;
		}
		
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////EXIT HANDLER////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

void * signal_thread(void* arg){

	int sig_caught;
	int rc;

	rc = sigwait(&(global_utilities.signal_mask), &sig_caught); //Catch SIGINT or SIGTERM
	global_utilities.accepting = 0; //Stop the Main Thread from accepting new connections

	if(rc != 0)
		printf("EXIT HANDLER: - Sigwait failed: %s\n",strerror(errno));

	switch(sig_caught){
		case SIGINT:
			if(global_utilities.active_clients > 0) 
				printf("EXIT HANDLER: - We will have to wait for all active clients to get served!\n");
			while(global_utilities.active_clients > 0) sleep(1);
			close(global_utilities.server_socket);
			printf("EXIT HANDLER: - All active clients have been dealt with!\n Let's free resources!\n");
			destroyQueueP(&(global_utilities.queueOfPairs));
			destroyQueue(&(global_utilities.queueOfItems));
			destroyWorkerResources();
			break;
		case SIGTERM:
			if(global_utilities.active_clients > 0) 
				printf("EXIT HANDLER: - We will have to wait for all active clients to get served!\n");
			while(global_utilities.active_clients > 0) sleep(1);
			close(global_utilities.server_socket);
			printf("EXIT HANDLER: - All active clients have been dealt with!\n Let's free resources!\n");
			destroyQueueP(&(global_utilities.queueOfPairs));
			destroyQueue(&(global_utilities.queueOfItems));
			destroyWorkerResources();		
			break;
		default:
			printf("\nEXIT HANDLER: - Unexpected signal %d\n",sig_caught);
			break;
	}
	printf("EXIT HANDLER: - Server will terminate in 1 second!\n");
	sleep(1);
	printf("SERVER: SEE YA!\n");
	exit(1);
}
