//Author: Nicholas A. Pfister
//pthread p2p message example
//run command: ./p2p_msg_client PORTNUM NETWORK_TOPOLOGY_FILE 

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//networking includes
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//includes not relevant to project
#include <sys/time.h>
#include <unistd.h>
//graph includes
#include "graph.h"
//message includes
#include "packet_types.h"

#define nthreads 2 // number of threads to spawn
#define receive 0 //thread 0 is receiver
#define send 1 //thread 1 is sender
#define log_size 10 //num chars in log file name
#define serv_name_size 20 //max num chars in server name
#define rcv_buff_size 255 //max UDP message size

#define RCV_QUEUE_SIZE 16
#define SEND_QUEUE_SIZE 16

void * receiver (void * param);
void * transmitter (void * param);

// generic message type for send/rcv queues
typedef struct {
  int size;
  unsigned char data[rcv_buff_size];
} nc_message_t;

// Global Send and rcv queues
nc_message_t send_queue[SEND_QUEUE_SIZE];
nc_message_t rcv_queue[RCV_QUEUE_SIZE];
pthread_mutex_t sq_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rq_lock = PTHREAD_MUTEX_INITIALIZER;
int sq_head;
int sq_tail;
int rq_head;
int rq_tail;

inline int queue_full (int head, int tail, int size) { return ((head+1)%size == tail);}
inline int queue_empty (int head, int tail, int size) { return head == tail;}

long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

// Queue and Dequeue for circular buffers
int enqueue(pthread_mutex_t *lock, nc_message_t *queue, nc_message_t message, int *head, int *tail, int size) {
  int success = 1;

  pthread_mutex_lock(lock);

  // critical section
  if(queue_full(*head, *tail, size)) {
    success = 0;
  } else {
    queue[*head] = message;
    *head = (*head+1)%size; 
  }

  pthread_mutex_unlock(lock);

  return success;
}

nc_message_t dequeue(pthread_mutex_t *lock, nc_message_t *queue, int *head, int *tail, int size) {
  nc_message_t message;

  message.size = 0; // indicates no message

  pthread_mutex_lock(lock);

  // critical section
  if(!queue_empty(*head, *tail, size)) {
    message = queue[*tail];
    *tail = (*tail+1)%size; 
  }

  pthread_mutex_unlock(lock);

  return message;
}

void send_route_update() {
  //TODO
}

//struct to pass multiple things to entry function
typedef struct params params_t;
struct params {
	int sleep_time;
	pthread_rwlock_t file_lock;
	char file_name[log_size];//log file name
	int port_num;//this client's port number
	int dest_port;//destination port number
	char serv_name[serv_name_size];
};

int main(int argc, char *argv[])
{
	int i;//loop variable
	pthread_t tid[nthreads];//threads tid=thread ID
	params_t params[nthreads];//param structs that i last 
	pthread_rwlock_t file_lock;//file thread safe lock, not process safe
  // Graph Variables for Widest Path
  graph_t *graph;
  long long switch_alive_time[MAX_SWITCHES];
  long long curr_time;
  //temp receive message
  nc_message_t curr_message;

  if(argc < 3) {
    printf("Usage:\n%s <port_num> <network_topology_file>\n", argv[0]);
    return EXIT_FAILURE;
  }

	if(pthread_rwlock_init(&file_lock, NULL))//init lock
	{printf("Error creating file lock\n");return 2;}	
	
  //random number init
	srand((unsigned)time(NULL));

  //init queues
  rq_head = 0;
  sq_head = 0;
  rq_tail = 0;
  sq_tail = 0;

	//clear log file
	FILE * file;
	if((file=fopen("test.txt","w")) == NULL)
		exit(-5);
	fclose(file);

	//create threads
	//receiver
	params[receive].sleep_time = (rand() % 10) + 1;//1 to 10
	strncpy(params[receive].file_name, "test.txt",log_size);//no buff overflow
	params[receive].file_lock  = file_lock;
	params[receive].port_num = atoi(argv[2]);//portnum
	params[receive].dest_port= atoi(argv[3]);//dest portnum
	strncpy(params[receive].serv_name,argv[1],serv_name_size);//will not buffer overflow

	//threads default to joinable state, not detached
	//create reveiver
	if(pthread_create(&tid[receive],NULL,receiver,&params[receive]))
	{printf("Error creating thread\n");	exit(-1);}

	//transmitter
	params[send].sleep_time = (rand() % 10) + 1;//1 to 10
	strncpy(params[send].file_name, "test.txt",log_size);//no buff overflow
	params[send].file_lock  = file_lock;
	params[send].port_num = atoi(argv[2]);//portnum
	params[send].dest_port= atoi(argv[3]);//dest portnum
	strncpy(params[send].serv_name,argv[1],serv_name_size);//will not buffer overflow

	//threads default to joinable state, not detached
	//create reveiver
	if(pthread_create(&tid[send],NULL,transmitter,&params[send]))
	{printf("Error creating thread\n");	exit(-9);}


	//THREADS DOING WORK   **********************************

  // Read Network Topology File
  if((graph = create_graph_from_file(argv[3])) == NULL) {
    printf("Error: Couldn't create graph from file: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  // Set all switch last alive message times to now
  switch_alive_time[0] =  current_timestamp();
  for(i = 1; i < MAX_SWITCHES; i++) {
    switch_alive_time[i] = switch_alive_time[0];
  }
  
  // Respond to Message queue 
  while (1) {
    // React to any pending messages
    do {
      curr_message = dequeue(&rq_lock, rcv_queue, &rq_head, &rq_tail, RCV_QUEUE_SIZE);
      
      if(curr_message.size != 0) {
        //TODO: react to message 

      }

    } while(curr_message.size != 0);
    
    // Check to see if any dead nodes exist
    curr_time = current_timestamp();
    for(i=0; i < graph->size; i++) {
      if(graph->adj_list[i].active){ // check active links 
        if((curr_time - switch_alive_time[i]) > K_SEC*M_MISSES*1000) {
          // found dead link
          deactivate_switch(graph, i); 
          send_route_update();
        }
      }
    } 
    
  }

  // while dead nodes exist
  //    react to dead nodes
  //   

	//wait for threads to finish
	for (i = 0; i < nthreads; ++i)
	{
		pthread_join(tid[i],NULL);
	}
	pthread_rwlock_destroy(&file_lock);//destroy file_lock
	printf("ALL THREADS EXITED --- DONE\n");

	return 0;
}


/****************************************************
 *
 *  RECEIVE THREAD
 *
 * **************************************************/


void * receiver (void * param){
	//vars
	int my_tid = pthread_self();//thread ID
	params_t * params_ptr = (params_t*) param;//receive struct that is passing parameters
	params_t params = *params_ptr;
	//FILE *file;//log file
	
	//UDP vars
	int udp_fd, bytes_received, serverlength;
	struct sockaddr_in server_addr;
	char rcvbuffer[rcv_buff_size];
	
	//SETUP UDP
	//get socket file descriptor
	if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) //UDP
    	exit(-2);//UDP FAILED
	
	//fill sockaddr_in
	memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);//receiver listens to any IP address
	server_addr.sin_port = htons(params.port_num);//port number of this CLIENT/RECEIVER process
	
	//bind port to this process
	serverlength = sizeof(server_addr);
	if(bind(udp_fd,(struct sockaddr *)  &server_addr,  serverlength) < 0){
		printf("BIND FAILED\n");
		exit(-6);
	}

	//END UDP SETUP

	//print received data to stdout and log file
	//if((file=fopen(params.file_name,"a")) == NULL)//open to append
	//	exit(-4);
	while(1){
	bytes_received = recvfrom(udp_fd,rcvbuffer,rcv_buff_size,0, (struct sockaddr *) &server_addr, (socklen_t *) &serverlength);
	rcvbuffer[bytes_received]='\0';
	fprintf(stdout, "%s\n", rcvbuffer);
	}
	//CAREFUL recvfrom resets server_addr every time, figure out how to repeatedly receive

	close(udp_fd);
	//fclose(file);
	return 0;
}


/****************************************************
 *
 *  SEND THREAD
 *
 * **************************************************/


void * transmitter (void * param){
	//vars
	int my_tid = pthread_self();//thread ID
	params_t * params_ptr = (params_t*) param;//receive struct that is passing parameters
	params_t params = *params_ptr;
	//FILE *file;//log file
	
	//UDP vars
	int udp_fd, bytes_sent,serverlength;
	struct hostent * server;
	struct sockaddr_in server_addr;
	char sendbuffer[rcv_buff_size];
	struct in_addr **dest_addresses;

	//SETUP UDP
	//get socket file descriptor
	if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) //UDP
    	exit(-2);//UDP FAILED
	
	//retrieve address from given hostname
	if ((server = gethostbyname(params.serv_name)) == NULL){
		printf("SERVER: %s not found", params.serv_name);
		exit(-3);
	}
	dest_addresses = (struct in_addr **) server->h_addr_list;
	printf("%s\n", inet_ntoa(*dest_addresses[0]));

	//fill sockaddr_in
	memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr,server->h_length);//dest to send to for sender/transmitter thread
	server_addr.sin_port = htons(params.dest_port);//port number to send to	
	
	//get input from command line
	while(1){
	fgets(sendbuffer,rcv_buff_size,stdin);
	serverlength = sizeof(server_addr);
	bytes_sent= sendto(udp_fd,sendbuffer,strlen(sendbuffer),0, (struct sockaddr *) &server_addr,serverlength);
	if(bytes_sent < 0){
		printf("sendto failed\n");
		exit(-7);
	}
	}
	close(udp_fd);

	return 0;
}

