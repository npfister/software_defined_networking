//Author: Nicholas A. Pfister
//pthread example, warm up

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//includes not relevant to project
#include <time.h>
#include <unistd.h>

// number of threads to spawn
#define nthreads 2

//void * receiver (void * param);
//void * transmitter (void * param);
void * dummy_entry_pt (void * param);

int main(int argc, char const *argv[])
{
	pthread_t tid[nthreads];//threads tid=thread ID
	int sleep_time[nthreads];
	int i;
	//random number init
	srand((unsigned)time(NULL));

	//eventual loop to create
	for (i = 0; i < nthreads; ++i)
	{	
		sleep_time[i] = (rand() % 10) + 1;//1 to 10
		//threads default to joinable state not detached
		if(pthread_create(&tid[i],NULL,dummy_entry_pt,&sleep_time[i]))
		{
			printf("Error creating thread\n");
			return 1;
		}
	}
	
	//wait for threads to finish
	for (i = 0; i < nthreads; ++i)
	{
		pthread_join(tid[i],NULL);
	}
	printf("ALL THREADS EXITED --- DONE\n");

	return 0;
}

//in send / receiver per switch should use pthread locks to access writing to logs
void * dummy_entry_pt (void * param)
{
	int my_tid = pthread_self();
	int * sleep_time = (int *) param;
	printf("PID:%u Sleep for %d S\n", my_tid, (int) *sleep_time);
	sleep(*sleep_time);
	printf("PID:%u Woke Up, Exiting\n", my_tid);

	return NULL;//nothing of value to return in this example
}