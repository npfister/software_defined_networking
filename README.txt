DIRECTORY CONTENTS:

docs - provided by course
Examples - provided by course
source - source code written for the sdn project
inputs - graph topology files


SOURCE CONTENTS:

graph.c                   graph implemenation
graph.h                   header for graph implementation
graph_test.c              program testing graph and widest path functionality
Makefile                  builds the controller and switch
                                make switch //builds switch
                                make sdn_controller //builds the controller
network_controller.c      main file for the sdn controller
p2p_msg_client.c          Base udp sender / receiver
packet_types.h            Defines the message types and shared defines between the 
                          sdn controller and switches
priority_queue.c          Implemenation of heap based priority queue 
priority_queue.h          header for priority queue implementation
pthreads_with_logging.c   test for logging with multiple threads
pthread_warm_up.c         test for thread creation
start_switches.bash       Script for creating switches.  Outputs a bash script
                          that kills any created switches.  This file must be
                          updated for the number of switches in the network.
switch_sdn.c              main file for the switch



SWITCH USAGE:

To build:
make switch

run command: ./switch_sdn <switchID> <controller hostname> <controller port>
run command: ./switch_sdn <switchID> <controller hostname> <controller port> -f <neighbor ID>
run command: ./switch_sdn <switchID> <controller hostname> <controller port> -l
run command: ./switch_sdn <switchID> <controller hostname> <controller port> -f <neighbor ID_0> -l -f <neighbor ID_1>
 -l = log all messages sent and received, usually keep alives (in or out) are not logged
 -f <neighbor ID> = neighbor switch ID who's link is dead, although the switch itself is alive



CONTROLLER USAGE:

To build the sdn controller:
make sdn_controller

Usage:
./sdn_controller <port> <topology_file>





//NICK PFISTER contributions

1) Wrote UDP threaded (pthread) receiver and sender framework,
   that switch_sdn and sdn_controller were develop from into their final product

2) Wrote and debugged multi-threaded "switch_sdn"



//John Skubic contributions

1) Write code to create and manage graphs
  a) each edge and vertex has its own active flag to easily "turn off" the edges and vertices

2) Wrote the code to perform the widest path algorithm
  a) uses a priority queue implemeted as a heap

3) Modified the base p2p udp program created by Nick to add controller functionality
  a) keeps track of alive switches and marks them dead if the topology update message hasn't been sent recently
  b) updates graph with reported alive and dead links through the topology update messages
  c) updates graph with dead and alive switches 
  d) Builds and sends routing tables for each switch based off the widest path algorithm every time the status
      of a link or switch is updated

