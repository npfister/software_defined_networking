#gcc -std=c99 -Wall -Wshadow -lpthread pthread_warm_up.c -o pthread_warm_up

CC=gcc
CFLAGS= -Wall -Wshadow -pthread -g

all: clean pthread_warm_up pthreads_with_logging p2p switch_sdn

clean:
	rm -rf pthread_warm_up pthreads_with_logging p2p_msg_client test.txt *.o graph_test *.gch sdn_controller switch_*.txt controller.txt killproc.bash switch_sdn

pthread_warm_up:
	$(CC) $(CFLAGS) pthread_warm_up.c -o pthread_warm_up

pthreads_with_logging:
	$(CC) $(CFLAGS) pthreads_with_logging.c -o pthreads_with_logging

#peer to peer chat box application w/ generic tranceivers
#to be adapted into clients and server for SDN project
p2p:
	$(CC) $(CFLAGS) p2p_msg_client.c -o p2p_msg_client

switch:
	$(CC) $(CFLAGS) switch_sdn.c -o switch_sdn

sdn_controller: network_controller.o graph.o priority_queue.o
	$(CC) $(CFLAGS) -o sdn_controller network_controller.o graph.o priority_queue.o

network_controller.o: network_controller.c graph.h packet_types.h
	$(CC) $(CFLAGS) -c network_controller.c graph.h packet_types.h

graph: graph_test.o graph.o priority_queue.o
	$(CC) $(CFLAGS) -o graph_test graph_test.o graph.o priority_queue.o

graph_test.o: graph_test.c graph.h
	$(CC) $(CFLAGS) -c graph_test.c graph.h

graph.o: graph.c graph.h
	$(CC) $(CFLAGS) -c graph.c graph.h

priority_queue.o: priority_queue.c priority_queue.h
	$(CC) $(CFLAGS) -c priority_queue.c priority_queue.h

