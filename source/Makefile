#gcc -std=c99 -Wall -Wshadow -lpthread pthread_warm_up.c -o pthread_warm_up

CC=gcc
CFLAGS= -Wall -Wshadow -pthread

clean:
	rm -rf pthread_warm_up

pthread_warm_up:
	$(CC) $(CFLAGS) pthread_warm_up.c -o pthread_warm_up

pthread_with_logging:
	$(CC) $(CFLAGS) pthread_with_logging.c -o pthread_with_logging