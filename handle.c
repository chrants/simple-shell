#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "util.h"



void
sigint_handler() 
{
	ssize_t bytes;
	bytes = write(STDOUT_FILENO, "Nice try.\n", 10);
	if(bytes != 10)
		exit(-999);
}

void
sigusr1_handler()
{
	ssize_t bytes;
	bytes = write(STDOUT_FILENO, "exiting\n", 8);
	if(bytes != 8)
		exit(-999);
	exit(1);
}


/*
* First, print out the process ID of this process.
*
* Then, set up the signal handler so that ^C causes
* the program to print "Nice try.\n" and continue looping.
*
* Finally, loop forever, printing "Still here\n" once every
* second.
*/
int main(int argc, char **argv)
{
	pid_t pid = getpid();
	printf("%d\n", pid);

	Signal(SIGINT, sigint_handler);
	while(1) {
		struct timespec time_left;
		time_left.tv_sec = 1;
		while(nanosleep(&time_left, &time_left) == -1);
		fprintf(stdout, "Still here\n");
	}

	return 0;
}


