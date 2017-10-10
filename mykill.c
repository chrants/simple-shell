#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>


int main(int argc, char **argv)
{
  if(argv[1] == NULL) {
    printf("%s", "No process ID specified.\n");
    exit(1);
  }
  
  kill(atoi(argv[1]), SIGUSR1);
  return 0;
}
