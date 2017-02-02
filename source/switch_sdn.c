//Author: Nicholas A. Pfister
//SDN client switch
//run command: ./switch_sdn <switchID> <controller hostname> <controller port>
//run command: ./switch_sdn <switchID> <controller hostname> <controller port> -f <neighbor ID>
//run command: ./switch_sdn <switchID> <controller hostname> <controller port> -l
//run command: ./switch_sdn <switchID> <controller hostname> <controller port> -f <neighbor ID_0> -l -f <neighbor ID_1>
// -l = log all messages sent and received, usually keep alives (in or out) are not logged
// -f <neighbor ID> = neighbor switch ID who's link is dead, although the switch itself is alive

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
//networking includes
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//sdn control message types
#include <packet_types.h> //make packet.c to rip off message type, should be universal to server and switch


#define nthreads 2 // number of threads to spawn
#define receive 0 //thread 0 is receiver
#define send 1 //thread 1 is sender
#define log_size 20 //num chars in log file name
#define serv_name_size 20 //max num chars in server name
#define rcv_buff_size 255 //max UDP message size
#define trans_slp_intrv 20 // percent of a second defining granularity sender thread checks for dead switches/send kalives

void * receiver (void * param);
void * transmitter (void * param);

//struct to pass multiple things to entry function
typedef struct params params_t;
struct params {
	pthread_rwlock_t file_lock;
	pthread_mutex_t swb_mutex;//lock for switch info struct
	pthread_cond_t registered;//conditional wait for receiver to signal transmitter register response has come and time to start kalives
	char file_name[log_size];//log file name
	int port_num;//this server's port number
	int ctrl_port;//server port number
	char serv_name[serv_name_size];
};
typedef struct sw_info switch_info_t;
struct sw_info {
	int last_kalive[MAX_NEIGHBORS];//time() return signed
	char neighbor_id[MAX_NEIGHBORS]; // -1 -> invalid entry
	unsigned char active_flag[MAX_NEIGHBORS];//active high, switch is alive or dead
	unsigned char link_alive[MAX_NEIGHBORS];//active high,overrides activeflag, "emulate dead links", do not send keep alives to these switches
	unsigned char host[MAX_NEIGHBORS];
	unsigned char port[MAX_NEIGHBORS];
};


//global vars
unsigned char my_swID;//this switch's ID
unsigned char log_level;//0 = minimal, anything else = log all in/out keepalive messages

switch_info_t switch_board;// :)      -   must use mutex to 
unsigned char route_table[MAX_SWITCHES];//this switch's routing table

int main(int argc, char const *argv[])
{
	//VARIABLES
	int i;//loop variable
	pthread_t tid[nthreads];//threads tid=thread ID
	params_t params[nthreads];//param structs that i last
	pthread_cond_t registered;
	if(pthread_cond_init(&registered,NULL))
		{printf("Error creating registered cond_t signal\n");exit(-12);}
	pthread_mutex_t swb_mutex;//lock for switch info struct
	if(pthread_mutex_init(&swb_mutex,NULL))
		{printf("Error creating switch mutex\n");exit(-11);}
	pthread_rwlock_t file_lock;//file thread safe lock, not process safe
	if(pthread_rwlock_init(&file_lock, NULL))//init lock
		{printf("Error creating file lock\n");exit(-10);}	
	//random number init
	srand((unsigned)time(NULL));

	//INITIALIZATIONS
	log_level=0;//default to logging everthing but keepalives
	my_swID = (unsigned char) 
	for(i=0,i<MAX_NEIGHBORS,i++){
		switch_board.last_kalive[i]= -1;//indicate that a kalive for this neighbor has not been received
		switch_board.neighbor_id[i]= -1;//invalid entry
		switch_board.active_flag[i]=  0;//0 is inactive, 1 is active
		switch_board.link_alive [i]=  1;//assume links alive unless cmdline says otherwise
	}
	//init log filename  --  snprintf prevents buffer overflows
	snprintf(params[receive].file_name,log_size, "switch_%d.txt", atoi(argv[1]));//argv[1] is switchID
	strncpy(params[send].file_name, params[receive].file_name,log_size);//no buff overflow

	//clear log file
	FILE * file;
	if((file=fopen(params[receive].file_name,"w")) == NULL)
		exit(-5);
	fclose(file);

	//CMD LINE OPTIONS
	i=0;
	while( optind < argc){
		i++;
		if((i=getopt(argc, argv, "f:i")) != -1){
			switch(i){
				case 'f': {
					//just put switchID in link alive,
					//correct when you get neighbors after register response
					switch_board.link_alive[i]= (unsigned char) atoi(optarg);//put switchID in array
				}
				case 'i': {
					log_level=1;//will cause logging of in/out keepalives also
				}
				default:
					break;
			}
		}
	}

	//CREATE THREADS
	//receiver
	params[receive].registered = registered;
	params[receive].file_lock = file_lock;
	params[receive].swb_mutex = swb_mutex;
	params[receive].port_num = 1024 + (rand() % 500) + (time() % 500);//this switch's portnum, 1024+ to get above well known ports, port will be b/t 1024 and 2024
	params[receive].ctrl_port= atoi(argv[3]);//controller's portnum
	strncpy(params[receive].serv_name,argv[2],serv_name_size);//will not buffer overflow

	//threads default to joinable state, not detached
	//create reveiver
	if(pthread_create(&tid[receive],NULL,receiver,&params[receive]))
	{printf("Error creating thread\n");	exit(-1);}

	//transmitter
	params[send].registered = registered;
	params[send].file_lock  = file_lock;
	params[send].swb_mutex = swb_mutex;
	params[send].port_num = params[receive].port_num;//this switch's portnum
	params[send].ctrl_port= atoi(argv[3]);//controller's portnum
	strncpy(params[send].serv_name,argv[2],serv_name_size);//will not buffer overflow

	//threads default to joinable state, not detached
	//create transmitter
	if(pthread_create(&tid[send],NULL,transmitter,&params[send]))
	{printf("Error creating thread\n");	exit(-9);}

	//THREADS DOING WORK   **********************************

	//wait for threads to finish
	for (i = 0; i < nthreads; ++i)
	{
		pthread_join(tid[i],NULL);
	}
	pthread_cond_destroy(&registered);//destroy conditional signal
	pthread_mutex_destroy(&swb_mutex);//destroy mutex
	pthread_rwlock_destroy(&file_lock);//destroy file_lock
	printf("ALL THREADS EXITED --- DONE\n");

	return 0;
}

//************
	//receiver 
		//initially
			//block for REGISTER_RESPONSE
				//copy neighbor_id
					//as do this check if any match 
				//copy active_flags
				//copy host
				//copy port
				//send unblock to sender

		//LOOP:

		//receive TOPOLOGY_UPDATE
			//copy routing table into my local routing table

		//receives KEEP_ALIVE from switch
			//get struct lock
			//last_kalive[switchID] = time()
			//if(active_flag[switchID] == FALSE)
				//send_topo_update = TRUE;//sender clears
				//active_flag[switchID] = TRUE;
			//release struct lock
//************
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

	//block for REGISTER_RESPONSE

	//once received
	pthread_mutex_lock(&params.swb_mutex);
	pthread_cond_signal(&params.registered);//tell sender thread we have finished registration with server
	pthread_mutex_unlock(&params.swb_mutex);

	//print received data to stdout and log file
	//if((file=fopen(params.file_name,"a")) == NULL)//open to append
	//	exit(-4);
	while(1){
	bytes_received = recvfrom(udp_fd,rcvbuffer,rcv_buff_size,0, (struct sockaddr *) &server_addr, (socklen_t *) &serverlength);
	rcvbuffer[bytes_received]='\0';
	fprintf(stdout, "%s\n", rcvbuffer);
	}
	

	close(udp_fd);
	//fclose(file);
	return 0;
}


//************
	//sender = transmitter
		//initially
			//send REGISTER_REQUEST
				//fill sender_id
				//fill host
				//fill port
				//send to server
		
		//block until receiver thread signals REGISTER_RESPONSE has been received
	
		//LOOP:

			//get struct lock
			//if ((current_time() - last_kalive[switchIDs] >= m*k)& active_flag[switchID]=TRUE) //FOR ALL SWITCHIDs
				//send_topo_update = TRUE;
				//active_flag[switchID] = FALSE; //that way will not send topology update again if switch is still dead
			//if (send_topo_update == TRUE) //***HANDLES switches that have died (sig from > m*k) or come back alive (sig from receiver)
				//send TOPOLOGY_UPDATE to controller
				//send_topo_update = FALSE;//clear flag
			//if (current_time() - my_last_kalive_sent)
				//send kalives to all "connected to" switches 
				//do not send kalives to switches who are marked link_alive = FALSE or negatvie
				//my_last_kalive_sent = time()
			//release struct lock
			//sleep(trans_slp_intrv);//(tenth second);don't need to constantly checking switches are dead, several times per second will do
			//OR pthread_yield();//should work, gives receive a chance to run, but not much sleeping

//************
void * transmitter (void * param){
	//variable for last transmit
	//if current time - last transmit >= to K,
		//then transmit KEEP_ALIVE to all !link_dead and alive switchIDs, update last_transmit = time()

	//inside transmitter loop, sleep for 1/10ths of seconds each iteration to allow receive thread to get lock

	//lock neighbors stucture, unlock per iteration

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
	
	//UDP setup done

	//send REGISTER_REQUEST

	//block until receiver tells me REGISTER_RESPONE received
	pthread_mutex_lock(&params.swb_mutex);
	pthread_cond_wait(&params.registered,&params.swb_mutex);
	pthread_mutex_unlock(&params.swb_mutex);

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

//***************
//create shared memory neighbors/routing table structure for receive thread to update, and send thread to read from
//main will set flag to indicate whether a linkID has been marked dead by command line input
//threads will lock on properties structure:
	//bool send_topo_update //when a dead switch comes alive again receiver marks TRUE, after completion sender clears
//per switchID:
	//int_64 last_kalive //receiver sets this as the time() when a keepalive is received 
					   //sender references to determine if switch is DEAD (m*k) and 
					       //should mark send_kalive false and send controller TOPOLOGY UPDATE

	//bool link_dead // set by main() upon program start (from -f dead links on command line)
				   // if dead keepalives are not sent down this link (OVERRIDES EVERYTHING)

	//means I think that switch/neighbor is live
	//bool alive       //sender sends this switchID a KEEP_ALIVE if true
					 //if false, sender has already marked switch dead
					 //set false if (current_time - last_kalive_) > m*k

//**************

