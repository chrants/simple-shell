#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

const int MAX = 13;

static void doFib(int n, int doPrint);



/*
* unix_error - unix-style error routine.
*/
inline static void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}


int main(int argc, char **argv)
{
    int arg;
    int print=1;
    
    if(argc != 2){
        fprintf(stderr, "Usage: fib <num>\n");
        exit(-1);
    }
    
    arg = atoi(argv[1]);
    if(arg < 0 || arg > MAX){
        fprintf(stderr, "number must be between 0 and %d\n", MAX);
        exit(-1);
    }
    
    doFib(arg, print);
    
    return 0;
}


/*
 * Wrapper for catching fork() errors. 
 */
inline static pid_t Fork() {
    pid_t pid = fork();
    if(pid == -1) {
        unix_error("Did not successfully fork.");
    }
    return pid;
}

/* 
* Recursively compute the specified number. If print is
* true, print it. Otherwise, provide it to my parent process.
*
* NOTE: The solution must be recursive and it must fork
* a new child for each call. Each process should call
* doFib() exactly once.
*/
#define EXIT(n) if(doPrint) { printf("%d\n", n); } exit(n);
static void 
doFib(int n, int doPrint)
{
    if(n == 0 || n == 1) {
        EXIT(n);
    }
    
    int child1;
    int child2;
    
    if(!(child1 = Fork())) { // Child 1
        doFib(n-1, 0);
    } 
    if(!(child2 = Fork())) { // Child 2
        doFib(n-2, 0);
    }

    int sum = 0;
    int status;
    while(wait(&status) > 0) {
        if(WIFEXITED(status)) {
            sum += WEXITSTATUS(status);
        } else {
            unix_error("Child did not exit successfully.");
        }
    }
    EXIT(sum);
}
