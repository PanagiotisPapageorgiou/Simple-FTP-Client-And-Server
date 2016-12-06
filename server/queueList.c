#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "queueList.h"
#define QUEUE_SIZE 1000

//////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////The code here is self-explanatory so I will provide only sort descriptions///////
/////////////////////////Some descriptions can also be found in queueList.h///////////////////////////////

//The Queues have been implemented as linked lists

int initQueue(QueuePtr queueptr,int queue_size){ //Initialises Main Queue

	queueptr->items = 0;
	queueptr->Head = NULL;
	queueptr->Current = NULL;
	queueptr->max_size = queue_size;
	queueptr->working = 1;
}

void printQueue(Queue queue){

	int i=0;

	if(queue.items == 0){
		//printf("No items in the Queue to show!\n");
		return;
	}
	else{
		queue.Current = queue.Head;
		while(queue.Current != NULL){
			printf("Item:|%d|-Pathname:|%s|\n",i,queue.Current->pathname);
			queue.Current = queue.Current->next;
			i++;
		}
	}
}

void addQueue(QueuePtr queueptr,char* pathname,int client_socket,pthread_mutex_t* client_mtx){ //Move to the end and add a new node

	//printf("Attempting to add to queue this pathname:|%s|\n",pathname);
	if(queueptr->items == queueptr->max_size){
		//printf("Queue is already full! Try again later?\n");
		return;
	}
	else{
		if(queueptr->items == 0){
			//printf("Adding new Item as Head of the Queue!\n");
			queueptr->Head = malloc(sizeof(ItemNode));
			queueptr->Head->pathname = malloc(sizeof(char)*(strlen(pathname)+1));
			strcpy(queueptr->Head->pathname,pathname);
			queueptr->Head->client_socket = client_socket;
			queueptr->Head->client_mtx = client_mtx;
			queueptr->Head->next = NULL;
			(queueptr->items)++;
			queueptr->Current = queueptr->Head;
			//printf("Our current Head carries this pathname:|%s|\n",queueptr->Head->pathname);
		}
		else{
			queueptr->Current = queueptr->Head;
			while(queueptr->Current->next != NULL) queueptr->Current = queueptr->Current->next;
			//printf("Adding at the end of the Queue!\n");
			queueptr->Current->next = malloc(sizeof(ItemNode));
			queueptr->Current->next->pathname = malloc(sizeof(char)*(strlen(pathname)+1));
			strcpy(queueptr->Current->next->pathname,pathname);
			queueptr->Current->next->client_socket = client_socket;
			queueptr->Current->next->client_mtx = client_mtx;
			queueptr->Current->next->next = NULL;
			(queueptr->items)++;
			queueptr->Current = queueptr->Current->next;
			//printf("Our latest addition to the queue has this pathname:|%s|\n",queueptr->Current->pathname);
		}
	}		
}

void removeElementOfQueue(QueuePtr queueptr,int target){ //Remove a specific node,count begins from 1 for first node,2 for second etc

	ItemNodePtr Helper;
	int i=0;

	if(target <= 0){
		//printf("Target numbering begins from 1!\n");
		return;
	}
	if(queueptr->items == 0){
		//printf("Queue is empty,nothing to remove!\n");
		return;
	}
	else if(queueptr->items < target){
		//printf("Target does not exist in this Queue!\n");
		return;
	}
	else{
		if(target == 1){
			queueptr->Current = queueptr->Head->next;
			free(queueptr->Head->pathname);
			free(queueptr->Head);
			queueptr->Head = queueptr->Current;
			(queueptr->items)--;
		}
		else{
			i = 2;
			queueptr->Current = queueptr->Head;
			Helper = queueptr->Current->next;
			while((i != target) && (Helper != NULL)){
				queueptr->Current = Helper;
				Helper = Helper->next;
				i++;
			}
			if(i == target){
				queueptr->Current->next = Helper->next;
				free(Helper->pathname);
				free(Helper);
				(queueptr->items)--;
			}
			else{
				printf("Target was not found?! WTF\n");
			}
		}
	}
}

void frontOfQueue(Queue queue,char** pathptr,int* clientptr){ //Never actually used - Creates a copy of the first node
	
	if(queue.items == 0){
		//printf("Queue has no items!\n");
		return;
	}
	*pathptr = malloc(sizeof(char)*(strlen(queue.Head->pathname)+1));
	strcpy(*pathptr,queue.Head->pathname);
	*clientptr = queue.Head->client_socket;
}

void destroyQueue(QueuePtr queueptr){ //Used to destroy completely a queue
	
	if(queueptr->items == 0){
		queueptr->Head = NULL;
		queueptr->Current = NULL;
		queueptr->working = 0;
		return;
	}
	while(queueptr->items > 0) removeElementOfQueue(queueptr,1);
	queueptr->Head = NULL;
	queueptr->Current = NULL;
	queueptr->working = 0;
	printf("Everything was deleted from Queue of Items!\n");

}

///////////////////////////////Same as above pretty much but for the Queue of Pairs////////////////////////////
//REMINDER: Queue of Pairs is a second Queue which containts pairs like this <Client_Socket,items_to_deliver>//
//If the program functions correctly the size of a Queue Of Pairs should be the same as the number of producers working
//since 1 producer == 1 client server

int initQueueP(QueueSocketItemsPtr queueptr){

	queueptr->items = 0;
	queueptr->Head = NULL;
	queueptr->Current = NULL;
}

void addQueueP(QueueSocketItemsPtr queueptr,int items_to_deliver,int client_socket){

	//printf("Attempting to add to queue this pathname:|%s|\n",pathname);
		int exists;

		if(queueptr->items == 0){
			//printf("Adding new Item as Head of the Queue!\n");
			queueptr->Head = malloc(sizeof(ClientTasksPair));
			queueptr->Head->items_to_deliver = items_to_deliver;
			queueptr->Head->client_socket = client_socket;
			queueptr->Head->next = NULL;
			(queueptr->items)++;
			queueptr->Current = queueptr->Head;
			//printf("Our current Head carries this pathname:|%s|\n",queueptr->Head->pathname);
		}
		else{
			exists = 0;
			exists = searchQueueP(queueptr,client_socket);
			if(exists == 1){
				printf("QueuePairs: A client has magically returned and reconnected and reentered the queue\nbefore we finished serving him!\n");
				return;
			}
			queueptr->Current = queueptr->Head;
			while(queueptr->Current->next != NULL) queueptr->Current = queueptr->Current->next;
			//printf("Adding at the end of the Queue!\n");
			queueptr->Current->next = malloc(sizeof(ClientTasksPair));
			queueptr->Current->next->items_to_deliver = items_to_deliver;
			queueptr->Current->next->client_socket = client_socket;
			queueptr->Current->next->next = NULL;
			(queueptr->items)++;
			queueptr->Current = queueptr->Current->next;
			//printf("Our latest addition to the queue has this pathname:|%s|\n",queueptr->Current->pathname);
		}	
}

void removeElementOfQueueP(QueueSocketItemsPtr queueptr,int target){

	ClientTasksPairPtr Helper;
	int i=0;

	if(target <= 0){
		//printf("Target numbering begins from 1!\n");
		return;
	}
	if(queueptr->items == 0){
		//printf("Queue is empty,nothing to remove!\n");
		return;
	}
	else if(queueptr->items < target){
		//printf("Target does not exist in this Queue!\n");
		return;
	}
	else{
		if(target == 1){
			queueptr->Current = queueptr->Head->next;
			free(queueptr->Head);
			queueptr->Head = queueptr->Current;
			(queueptr->items)--;
		}
		else{
			i = 2;
			queueptr->Current = queueptr->Head;
			Helper = queueptr->Current->next;
			while((i != target) && (Helper != NULL)){
				queueptr->Current = Helper;
				Helper = Helper->next;
				i++;
			}
			if(i == target){
				queueptr->Current->next = Helper->next;
				free(Helper);
				(queueptr->items)--;
			}
			else{
				printf("Target was not found?! WTF\n");
			}
		}
	}
}

void destroyQueueP(QueueSocketItemsPtr queueptr){

	if(queueptr->items == 0){
		queueptr->Head = NULL;
		queueptr->Current = NULL;
		return;
	}
	while(queueptr->items > 0) removeElementOfQueueP(queueptr,1);
	queueptr->Head = NULL;
	queueptr->Current = NULL;
	printf("Everything was deleted from Queue of Pairs!\n");
}

int searchQueueP(QueueSocketItemsPtr queueptr,int client_socket){ //See whether a specific client socket exists or not

	if(queueptr->items == 0) return 0;
	else{
		queueptr->Current = queueptr->Head;
		do{
			if(queueptr->Current->client_socket == client_socket) return 1;
			queueptr->Current = queueptr->Current->next;
		}while(queueptr->Current != NULL);
		return 0;
	}
}

int updateQueueP(QueueSocketItemsPtr queueptr,int client_socket,int items){ //Update the items_to_deliver value of a specific client

	if(queueptr->items == 0) return 0;
	else{
		queueptr->Current = queueptr->Head;
		do{
			if(queueptr->Current->client_socket == client_socket){
				queueptr->Current->items_to_deliver = items;
			 	return 1;
			}
			queueptr->Current = queueptr->Current->next;
		}while(queueptr->Current != NULL);
		return 0;
	}
}

int getItemsOfClientQueueP(QueueSocketItemsPtr queueptr,int client_socket){ //Return the number of items associated with a client_socket

	if(queueptr->items == 0) return -1;
	else{
		queueptr->Current = queueptr->Head;
		do{
			if(queueptr->Current->client_socket == client_socket){
				return queueptr->Current->items_to_deliver;
			}
			queueptr->Current = queueptr->Current->next;
		}while(queueptr->Current != NULL);
		return -1;
	}

}

int setItemsOfClientQueueP(QueueSocketItemsPtr queueptr,int client_socket,int items){ //Change the number of items associated with a client_socket

	if(queueptr->items == 0) return -1;
	else{
		queueptr->Current = queueptr->Head;
		do{
			if(queueptr->Current->client_socket == client_socket){
				queueptr->Current->items_to_deliver = items;
				return 1;
			}
			queueptr->Current = queueptr->Current->next;
		}while(queueptr->Current != NULL);
		return -1;
	}

}

int getPositionOfClientQueueP(QueueSocketItemsPtr queueptr,int client_socket){ //Locate the position of a client_socket in the queue
																			   //useful for calling remove on it afterwards

	int i=1;

	if(queueptr->items == 0) return -1;
	else{
		queueptr->Current = queueptr->Head;
		do{
			if(queueptr->Current->client_socket == client_socket){
				return i;
			}
			queueptr->Current = queueptr->Current->next;
			i++;
		}while(queueptr->Current != NULL);
		return -1;
	}
}

