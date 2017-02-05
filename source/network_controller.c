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
#define rcv_buff_size 256 //max UDP message size

#define RCV_QUEUE_SIZE 32
#define SEND_QUEUE_SIZE 32


//********** CUSTOM TYPES *****************

// generic message type for send/rcv queues
typedef struct {
  int size;
  unsigned char data[rcv_buff_size];
  unsigned int host;
  int port;
} nc_message_t;

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

typedef struct {
  long long alive_time;
  unsigned int host;
  int port;
} switch_info_t;

 
//************** GLOBAL VARS *********************

// Global Send and rcv queues
nc_message_t    send_queue[SEND_QUEUE_SIZE];
nc_message_t    rcv_queue[RCV_QUEUE_SIZE];
pthread_mutex_t sq_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rq_lock = PTHREAD_MUTEX_INITIALIZER;
int             sq_head;
int             sq_tail;
int             rq_head;
int             rq_tail;

// Function Pointers
void * receiver (void * param);
void * transmitter (void * param);

// For File IO
pthread_rwlock_t  file_lock;//file thread safe lock, not process safe
FILE * file;

// ************** FUNCTION DEFINITIONS *****

void log_event(int id, void *message, int link_d0, int link_d1, int switch_d, int link_active) {
  
  register_req_t *reg_req = NULL;
  register_resp_t *reg_resp = NULL;
  route_update_t *r_update = NULL;
  int i = 0;



  while(pthread_rwlock_trywrlock(&file_lock)){} 
	if((file=fopen("test.txt","a")) == NULL)
		exit(-5);


  if(file != NULL) {

    switch (id) {

      case REGISTER_REQUEST :
        reg_req = (register_req_t*)message;
        fprintf(file, "\nController: Logging Register Request Message\n");
        fprintf(file, "Controller: switch_id: %d\nhost: %d\nport%d\n",
          reg_req->switch_id, reg_req->host, reg_req->port);
        break;

      case REGISTER_RESPONSE :
        i = 0;
        reg_resp = (register_resp_t*)message;
        fprintf(file, "\nController: Logging Register Response Message\n");
        while(reg_resp->neighbor_id[i] != -1 && i < MAX_NEIGHBORS) {
          fprintf(file, "Controller: Neighbor_id: %d\nactive: %d\nhost:%d\nport:%d\n",
            reg_resp->neighbor_id[i], reg_resp->active_flag[i], reg_resp->host[i],
            reg_resp->port[i]);
          i++;
        }
        break;

      case TOPOLOGY_UPDATE :
        i = 0;
        if(switch_d != -1) {
          fprintf(file, "\nController: Logging Topology Update with Switch Down\n");
          fprintf(file, "Controller: Switch down s: %d\n", switch_d);
        } else { // link down
          if(link_active) {
            fprintf(file, "\nController: Logging Topology Update with Link Activated\n");
            fprintf(file, "Controller: Link up betwween s: %d and s: %d\n", link_d0, link_d1);
          } else {
            fprintf(file, "\nController: Logging Topology Update with Link Down\n");
            fprintf(file, "Controller: Link down betwween s: %d and s: %d\n", link_d0, link_d1);
          }
        }
        break;

      case ROUTE_UPDATE :
        i = 0;
        r_update = (route_update_t*)message;
      
        fprintf(file, "\nController: Logging Route Update Message\n");
        fprintf(file, "Controller: Switch: %d\n", switch_d);
        while(r_update->route_table[i] != -2 && i < MAX_NEIGHBORS) {
          fprintf(file, "Controller: To switch: %d\nNext Hop: %d\n",
            i, r_update->route_table[i]);
          i++;
        }  
        break;
    
      default : 
        printf("Error: Invalid log id sent to logging function: %d\n", id);
        break;
    }
  } else {
    printf("Error: Invalid file descriptor sent to logging function.\n");
  }
  
  fclose(file);
  pthread_rwlock_unlock(&file_lock);

}


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
            
void fill_in_neighbors(register_resp_t *resp, graph_t *graph, switch_info_t *switch_info, int switch_id) {
  int j = 0;
  int i = 0;
  edge_t *curr_ptr = graph->adj_list[switch_id].next;

  while(curr_ptr != NULL) {
    if(switch_info[curr_ptr->vertex_conn].port != -1) { //is registered
      resp->neighbor_id[j] = curr_ptr->vertex_conn;
      resp->active_flag[j] = graph->adj_list[curr_ptr->vertex_conn].active;
      j++;
    }
    curr_ptr = curr_ptr->next;
  }

  for(i = 0; i < switch_id; i++) {
    curr_ptr = graph->adj_list[i].next;
    if(graph->adj_list[i].active) {
      while(curr_ptr != NULL) {
        if(curr_ptr->vertex_conn == switch_id) {
          if(switch_info[i].port != -1) { //is registered   
            resp->neighbor_id[j] = i;
            resp->active_flag[j] = graph->adj_list[i].active;
            j++;
          }
          break;
        }
        curr_ptr = curr_ptr->next;
      }
    }
  }

  resp->neighbor_id[j] = -1;

} 

void send_route_update(graph_t *graph, switch_info_t *switch_info) {
  int             i,j;
  route_update_t  rup;
  int             *table;
  nc_message_t    curr_message;
  
  rup.type = ROUTE_UPDATE;

  for(i = 0; i < MAX_SWITCHES; i++) {
    rup.route_table[i] = -1;
  }

  for(i = 0; i < graph->size; i++) {
    if(graph->adj_list[i].active) {
      table = build_routing_table(graph, i);
      for(j = 0; j < graph->size; j++) {
        rup.route_table[j] = (char)table[j];
      }
      rup.route_table[j] = -2;
      
      curr_message.size = sizeof(route_update_t);
      memcpy(curr_message.data, &rup, sizeof(route_update_t)); 
      curr_message.host = switch_info[i].host;
      curr_message.port = switch_info[i].port;
      log_event(ROUTE_UPDATE, (void*)&rup, -1, -1, i, 0);
      while (!enqueue(&sq_lock, send_queue, curr_message, &sq_head, &sq_tail, SEND_QUEUE_SIZE)) {}
      free(table);
    }
  }
}

//**************** MAIN ****************************

int main(int argc, char *argv[])
{
	int               i;//loop variable
  
  // Threading variables
	pthread_t         tid[nthreads];//threads tid=thread ID
	params_t          params[nthreads];//param structs that i last 


  // Graph Variables for Widest Path
  graph_t           *graph;
  switch_info_t     switch_info[MAX_SWITCHES];  
  long long         curr_time;          
  edge_t            *link_temp;
  int               needs_update;

  // message temporary vars
  nc_message_t      curr_message;
  unsigned char     curr_message_type;
  register_req_t    reg_req_temp;
  register_resp_t   reg_resp_temp;
  topology_update_t topology_update_temp;

  //*********** Initializations **************************

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
	if((file=fopen("test.txt","w")) == NULL)
		exit(-5);
	fclose(file);

  // Read Network Topology File
  if((graph = create_graph_from_file(argv[2])) == NULL) {
    printf("Error: Couldn't create graph from file: %s\n", argv[1]);
    return EXIT_FAILURE;
  }

  for(i = 0; i < MAX_SWITCHES; i++) {
    switch_info[i].alive_time = 0;
    switch_info[i].host = 0;
    switch_info[i].port = -1;
  }

  //********** BEGIN CREATE SEND AND RCV THREADS ************

	//create threads
	//receiver
	params[receive].sleep_time = (rand() % 10) + 1;//1 to 10
	strncpy(params[receive].file_name, "test.txt",log_size);//no buff overflow
	params[receive].file_lock  = file_lock;
	params[receive].port_num = atoi(argv[1]);//portnum
	//params[receive].dest_port= atoi(argv[3]);//dest portnum
	//strncpy(params[receive].serv_name,argv[1],serv_name_size);//will not buffer overflow

	//threads default to joinable state, not detached
	//create reveiver
	if(pthread_create(&tid[receive],NULL,receiver,&params[receive]))
	{printf("Error creating thread\n");	exit(-1);}

	//transmitter
	params[send].sleep_time = (rand() % 10) + 1;//1 to 10
	strncpy(params[send].file_name, "test.txt",log_size);//no buff overflow
	params[send].file_lock  = file_lock;
	//params[send].port_num = atoi(argv[2]);//portnum
	//params[send].dest_port= atoi(argv[3]);//dest portnum
	//strncpy(params[send].serv_name,argv[1],serv_name_size);//will not buffer overflow

	//threads default to joinable state, not detached
	//create reveiver
	if(pthread_create(&tid[send],NULL,transmitter,&params[send]))
	{printf("Error creating thread\n");	exit(-9);}

  //*************** END CREATE SEND AND RCV THREADS *********  

	//*************** THREADS DOING WORK   ********************
	
  //*************** BEGIN SDN CONTROLLER ********************

 
  // Temporary Test code:
  /*
  for(i = 0; i < graph->size; i++) {
    memset(&reg_req_temp, 0, sizeof(register_resp_t));
    reg_req_temp.type = REGISTER_REQUEST;
    reg_req_temp.switch_id = (unsigned char)i;
    reg_req_temp.host = i+1;
    reg_req_temp.port = i+2;
    curr_message.size = sizeof(register_req_t);
    memcpy(curr_message.data, &reg_req_temp, sizeof(register_req_t)); 
    curr_message.host = reg_req_temp.host;
    curr_message.port = reg_req_temp.port;
    while (!enqueue(&rq_lock, rcv_queue, curr_message, &rq_head, &rq_tail, RCV_QUEUE_SIZE)) {}
  }
  // Send topology updates after two seconds
  memset(&topology_update_temp, 0, sizeof(topology_update_t));
  topology_update_temp.type = TOPOLOGY_UPDATE;
  topology_update_temp.sender_id = 2;
  topology_update_temp.neighbor_id[0] = 5;
  topology_update_temp.active_flag[0] = 1;
  topology_update_temp.neighbor_id[1] = 1;
  topology_update_temp.active_flag[1] = 1;
  topology_update_temp.neighbor_id[2] = -1;
  curr_message.size = sizeof(topology_update_t);
  memcpy(curr_message.data, &topology_update_temp, sizeof(topology_update_t)); 
  while (!enqueue(&rq_lock, rcv_queue, curr_message, &rq_head, &rq_tail, RCV_QUEUE_SIZE)) {}
 
  topology_update_temp.sender_id = 0;
  topology_update_temp.neighbor_id[0] = 1;
  topology_update_temp.active_flag[0] = 1;
  topology_update_temp.neighbor_id[1] = 3;
  topology_update_temp.active_flag[1] = 1;
  topology_update_temp.neighbor_id[2] = -1;
  curr_message.size = sizeof(topology_update_t);
  memcpy(curr_message.data, &topology_update_temp, sizeof(topology_update_t)); 
  while (!enqueue(&rq_lock, rcv_queue, curr_message, &rq_head, &rq_tail, RCV_QUEUE_SIZE)) {}
  */
 
  // MAIN SDN CONTROLLER LOOP 
  while (1) {
    // React to any pending messages
    do {
      curr_message = dequeue(&rq_lock, rcv_queue, &rq_head, &rq_tail, RCV_QUEUE_SIZE);
      
      if(curr_message.size != 0) {
        curr_message_type = curr_message.data[0];

        switch (curr_message_type) {

          case REGISTER_REQUEST :
            memcpy(&reg_req_temp, curr_message.data, sizeof(register_req_t));
            memset(&reg_resp_temp, 0, sizeof(register_resp_t));

            activate_switch(graph,reg_req_temp.switch_id);
            switch_info[reg_req_temp.switch_id].port = curr_message.port;
            switch_info[reg_req_temp.switch_id].host = curr_message.host;
            switch_info[reg_req_temp.switch_id].alive_time = current_timestamp();

            log_event(REGISTER_REQUEST, (void*)&reg_req_temp, -1, -1, -1, 0);

            // send neighbors
            reg_resp_temp.type = REGISTER_RESPONSE;
            fill_in_neighbors(&reg_resp_temp, graph, switch_info, reg_req_temp.switch_id); 
            for(i = 0; i < graph->size; i++) {
              if(reg_resp_temp.neighbor_id[i] == -1)
                break;
      
              reg_resp_temp.host[i] = switch_info[(int)reg_resp_temp.neighbor_id[i]].host;
              reg_resp_temp.port[i] = switch_info[(int)reg_resp_temp.neighbor_id[i]].port;
            }      

            curr_message.size = sizeof(register_resp_t);
            memcpy(curr_message.data, &reg_resp_temp, sizeof(register_resp_t)); 
            curr_message.host = reg_req_temp.host;
            curr_message.port = reg_req_temp.port;
            log_event(REGISTER_RESPONSE, (void*)&reg_resp_temp, -1, -1, -1, 0);
            while (!enqueue(&sq_lock, send_queue, curr_message, &sq_head, &sq_tail, SEND_QUEUE_SIZE)) {}

            // send routing tables
            send_route_update(graph, switch_info);

            break;

          case TOPOLOGY_UPDATE :
            memcpy(&topology_update_temp, &curr_message.data, sizeof(topology_update_t));

            // set switch alive time
            switch_info[topology_update_temp.sender_id].alive_time = current_timestamp();

            // check to see if new link information
            needs_update = 0;
            for(i = 0; i < graph->size; i++) {
              if(topology_update_temp.neighbor_id[i] == -1) break;

              link_temp = find_link(graph, topology_update_temp.sender_id, topology_update_temp.neighbor_id[i]);

              if(link_temp == NULL) {
                printf("ERROR: Unexpected neighbor for sender id: %d and id: %d\n", topology_update_temp.sender_id,
                topology_update_temp.neighbor_id[i]);
              } else {
                if( link_temp->active != topology_update_temp.active_flag[i]) {
                  needs_update = 1;
                  link_temp->active = topology_update_temp.active_flag[i];
                  log_event(TOPOLOGY_UPDATE, NULL, topology_update_temp.sender_id,topology_update_temp.neighbor_id[i],
                    -1, topology_update_temp.active_flag[i]);
                }
              }
            }

            // if new info, send routing tables
            if(needs_update) {
              send_route_update(graph, switch_info);
            }
            break;
          default : 
            printf("Warning: Controller received unexpected message type: %d\n", curr_message_type); 
        }
      }

    } while(curr_message.size != 0);
    
    // Check to see if any dead nodes exist
    curr_time = current_timestamp();
    for(i=0; i < graph->size; i++) {
      if(graph->adj_list[i].active){ // check active links 
        if((curr_time - switch_info[i].alive_time) > K_SEC*M_MISSES*1000) {
          // found dead switch
          deactivate_switch(graph, i);
          switch_info[i].port = -1;
          switch_info[i].host = 0;
          log_event(TOPOLOGY_UPDATE, NULL, -1,-1, i, 0);
          send_route_update(graph, switch_info);
        }
      }
    } 
    
  }

  //**************** END SDN CONTROLLER ************************

  delete_graph(graph);

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
	//int my_tid = pthread_self();//thread ID
	params_t * params_ptr = (params_t*) param;//receive struct that is passing parameters
	params_t params = *params_ptr;
	//FILE *file;//log file
	
	//UDP vars
	int udp_fd, bytes_received, serverlength;
	struct sockaddr_in server_addr;
	char rcvbuffer[rcv_buff_size];
  
  //Message vars
  nc_message_t message;
	
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
	
  // host and port are ignored in receive queue
	message.port = 0;
  message.host = 0;
	while(1){
	  bytes_received = recvfrom(udp_fd,rcvbuffer,rcv_buff_size,0, (struct sockaddr *) &server_addr, (socklen_t *) &serverlength);
    message.size = bytes_received;
    message.port = ntohs(server_addr.sin_port);
    message.host = ntohs(server_addr.sin_addr.s_addr);
    memcpy(message.data, rcvbuffer, bytes_received);
    enqueue(&rq_lock, rcv_queue, message, &rq_head, &rq_tail, RCV_QUEUE_SIZE);
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
	
	//UDP vars
	int udp_fd, bytes_sent,serverlength;
	struct sockaddr_in server_addr;
	char sendbuffer[rcv_buff_size];

  //message vars
  nc_message_t message;

	//SETUP UDP
	//get socket file descriptor
	if ((udp_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) //UDP
    	exit(-2);//UDP FAILED
	
	//get input from command line
	while(1){
    do {
      message = dequeue(&sq_lock, send_queue, &sq_head, &sq_tail, SEND_QUEUE_SIZE);
    } while (message.size == 0);

	  //fill sockaddr_in
	  memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
	  server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = message.host; 
	  server_addr.sin_port = message.port;
    memcpy(sendbuffer, message.data, message.size);	

	  serverlength = sizeof(server_addr);
	  bytes_sent= sendto(udp_fd, sendbuffer, message.size,0, (struct sockaddr *) &server_addr,serverlength);
	  if(bytes_sent < 0){
	  	printf("sendto failed\n");
	  	//exit(-7);
	  }
	}
	close(udp_fd);

	return 0;
}

