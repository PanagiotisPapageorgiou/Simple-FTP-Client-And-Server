typedef struct ItemNode* ItemNodePtr;
typedef struct Queue* QueuePtr;
typedef struct ClientTasksPair* ClientTasksPairPtr;
typedef struct QueueSocketItems* QueueSocketItemsPtr;

typedef struct ItemNode{ //A Main Queue node
	char* pathname; //Item pathname
	ItemNodePtr next;
	int client_socket; //The client socket destination of the item
	pthread_mutex_t* client_mtx; //A pointer to the mutex for access to this socket
} ItemNode;

typedef struct Queue{
	ItemNodePtr Head;
	ItemNodePtr Current;
	int items;
	int max_size;
	int working; //Flag used for worker threads to understand
} Queue;		 //that the Main Queue no longer exists and detach themselves

typedef struct ClientTasksPair{ //Queue of pairs <Client_Socket,items_to_deliver>
	int items_to_deliver;
	int client_socket;
	ClientTasksPairPtr next;
} ClientTasksPair;

typedef struct QueueSocketItems{
	ClientTasksPairPtr Head;
	ClientTasksPairPtr Current;
	int items;
} QueueSocketItems;

/////////////////////////////////////////////////////////////
////////Main Queue functions////////////////////////////////
int initQueue(QueuePtr,int);
void printQueue(Queue queue);
void addQueue(QueuePtr,char*,int,pthread_mutex_t*);
void removeElementOfQueue(QueuePtr,int);
void frontOfQueue(Queue,char**,int*);
void destroyQueue(QueuePtr);

///////////////////////////////////////////////////////////
///////Queue of pairs functions///////////////////////////
int initQueueP(QueueSocketItemsPtr);
//void printQueueP(Queue queue);
void addQueueP(QueueSocketItemsPtr,int,int);
void removeElementOfQueueP(QueueSocketItemsPtr,int);
void destroyQueueP(QueueSocketItemsPtr);
int searchQueueP(QueueSocketItemsPtr,int);
int updateQueueP(QueueSocketItemsPtr,int,int);
int getItemsOfClientQueueP(QueueSocketItemsPtr,int);
int setItemsOfClientQueueP(QueueSocketItemsPtr,int,int);
int getPositionOfClientQueueP(QueueSocketItemsPtr,int);
