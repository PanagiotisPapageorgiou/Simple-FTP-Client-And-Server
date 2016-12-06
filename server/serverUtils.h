int checkNextDir(char*,int,Queue*,pthread_mutex_t*);
void perror_exit(char* msg);
int sendFile(char*,int,long);
char *name_from_address(struct in_addr);
char *itoa(int);
int write_all(int,void *,size_t);
int read_all(int,void*,size_t);
int fillTheQueue(char*,DIR*,int,Queue*,pthread_mutex_t*);
void* ServClCommunication(void*);
int sendOK(int);
int getOK(int,char*);
int argumentHandling(int,char**,int*,int*,int*);
void destroyWorkerResources();
int Transfer(int,char*,long);
void * Consumer(void*);
void * signal_thread(void*);

#define THREAD_POOL_SIZE_MAX 50

typedef struct ProducerArgs{ //Arguments of a producer
	int client_socket;
	int server_socket; //Not actually required since server_socket is accessible through Global_Utilities
} ProducerArgs;

typedef struct Global_Utilities{ //All shared data
	Queue queueOfItems; //The Main Queue
	pthread_t* worker_threads; //An array of worker thread variables
	pthread_cond_t cond_nonempty; //Condition variable for not empty Main queue
	pthread_cond_t cond_nonfull; //Condition variable for not full Main Queue
	//pthread_cond_t cond_delivered;
	pthread_mutex_t queue_mtx;	//Queue access mutex
	pthread_mutex_t readdir_mtx; //Readdir access mutex
	pthread_mutex_t malloc_mtx; //Malloc access mutex
	pthread_mutex_t free_mtx; //Free access mutex
	int thread_pool_size;
	int server_socket;
	int queue_size;
	int port;
	long blocksize;
	QueueSocketItems queueOfPairs; //A Queue of pairs <Client_Socket,Items_to_deliver>
	int active_workers;			   //Number of existing worker threads
	int active_clients;			   //Number of clients == number of producer threads
	int accepting;				   //Flag to control whether more clients can connect or not
	sigset_t signal_mask;		   //SIGSET mask used to catch SIGINT and SIGTERM and properly close the server
} Global_Utilities;
