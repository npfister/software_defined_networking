//Author: Nicholas A. Pfister
//pthread p2p message example
//run command: ./p2p_msg_client SERVER PORTNUM DEST_PORTNUM

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//networking includes
#include <sys/socket.h>
#include <netdb.h>
//includes not relevant to project
#include <time.h>
#include <unistd.h>

#define nthreads 1 // number of threads to spawn
#define receive 0 //thread 0 is receiver
#define send 1 //thread 1 is sender
#define log_size 10 //num chars in log file name
#define serv_name_size 20 //max num chars in server name
#define rcv_buff_size 150 //max UDP message size
//#define portnum 4321

void * receiver (void * param);
void * transmitter (void * param);

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

int main(int argc, char const *argv[])
{
	int i;//loop variable
	pthread_t tid[nthreads];//threads tid=thread ID
	params_t params[nthreads];//param structs that i last 
	pthread_rwlock_t file_lock;//file thread safe lock, not process safe
	if(pthread_rwlock_init(&file_lock, NULL))//init lock
	{printf("Error creating file lock\n");return 2;}	
	//random number init
	srand((unsigned)time(NULL));

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



	//THREADS DOING WORK   **********************************

	//wait for threads to finish
	for (i = 0; i < nthreads; ++i)
	{
		pthread_join(tid[i],NULL);
	}
	pthread_rwlock_destroy(&file_lock);//destroy file_lock
	printf("ALL THREADS EXITED --- DONE\n");

	return 0;
}

void * receiver (void * param){
	//vars
	int my_tid = pthread_self();//thread ID
	params_t * params_ptr = (params_t*) param;//receive struct that is passing parameters
	params_t params = *params_ptr;
	FILE *file;//log file
	
	//UDP vars
	int udp_fd;
	struct hostent * server;
	struct sockaddr_in server_addr;
	char rcvbuffer[rcv_buff_size];
	char * rcvbuffer_ptr;
	rcvbuffer_ptr = rcvbuffer;

	//SETUP UDP
	//get socket file descriptor
	if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) //UDP
    	exit(-2);//UDP FAILED
	
	//retrieve address from given hostname
	if ((server = gethostbyname(params.serv_name)) == NULL){
		printf("SERVER: %s not found", params.serv_name);
		exit(-3);
	}

	//fill sockaddr_in
	memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
	server_addr.sin_port = htons(params.dest_port);//port number of server
	server_addr.sin_family = AF_INET;
	memcpy(&server_addr.sin_addr, server->h_addr, sizeof(server_addr.sin_addr));//cpy in address of server

	//END UDP SETUP

	//print received data to stdout and log file
	if((file=fopen(params.file_name,"a")) == NULL)//open to append
		exit(-4);

	//while((recvfrom(udp_fd,rcvbuffer_ptr,rcv_buff_size,0, (struct sockaddr *) &server_addr.sin_addr)))
	//CAREFUL recvfrom resets server_addr every time, figure out how to repeatedly receive

	close(udp_fd);
	fclose(file);
	return 0;
}







//in send / receiver per switch should use pthread locks to access writing to logs
/*void * dummy_entry_pt (void * param)
{
	int my_tid = pthread_self();	
	params_t * params_ptr = (params_t*) param;//receive struct that is passing parameters
	params_t params = *params_ptr;
	FILE *file;
	file=fopen(params.file_name,"a");
	
	//get lock
	while(pthread_rwlock_trywrlock(&params.file_lock)){}
	fprintf(file, "PID:%u Sleep for %d S\n", my_tid, params.sleep_time );
	//release file_lock
	pthread_rwlock_unlock(&params.file_lock);

	printf("PID:%u Sleep for %d S\n", my_tid, (int) params.sleep_time);
	sleep(params.sleep_time);
	printf("PID:%u Woke Up, Exiting\n", my_tid);

	fclose(file);
	return NULL;//nothing of value to return in this example
}*/