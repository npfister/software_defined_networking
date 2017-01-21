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

//struct to pass multiple things to entry function
typedef struct params params_t;
struct params {
	int sleep_time;
	pthread_rwlock_t file_lock;
	char file_name[10];
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
	file=fopen("test.txt","w");
	fclose(file);

	//loop to create threads
	for (i = 0; i < nthreads; ++i)
	{	
		params[i].sleep_time = (rand() % 10) + 1;//1 to 10
		strcpy(params[i].file_name, "test.txt");
		params[i].file_lock = file_lock;

		//threads default to joinable state not detached
		if(pthread_create(&tid[i],NULL,dummy_entry_pt,&params[i]))
		{printf("Error creating thread\n");	return 1;}
	}
	
	//wait for threads to finish
	for (i = 0; i < nthreads; ++i)
	{
		pthread_join(tid[i],NULL);
	}
	pthread_rwlock_destroy(&file_lock);//destroy file_lock
	printf("ALL THREADS EXITED --- DONE\n");

	return 0;
}

//in send / receiver per switch should use pthread locks to access writing to logs
void * dummy_entry_pt (void * param)
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
}