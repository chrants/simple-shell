/* 
 * msh - A mini shell program with job control
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "util.h"
#include "jobs.h"


/* Global variables */
int verbose = 0;            /* if true, print additional output */

extern char **environ;      /* defined in libc */
static char prompt[] = "msh> ";    /* command line prompt (DO NOT CHANGE) */
static struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */
// void handle_child_returned(pid_t pid, int status);

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
void usage(void);
void sigquit_handler(int sig);



/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}

/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char *argv[MAXARGS];
    int bg = parseline(cmdline, argv);
     
    if(argv[0] == NULL) {
        return;
    }

    if(!builtin_cmd(argv)) {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, NULL);

        pid_t pid = Fork();
        if(pid == 0) {
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
            setpgid(0, 0);
            execve(argv[0], argv, environ);
            fprintf(stderr, "%s: Command not found\n", argv[0]);
        }
        
        if(bg) {
            addjob(jobs, pid, BG, cmdline);
            struct job_t *job = getjobpid(jobs, pid);
            printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
            sigprocmask(SIG_UNBLOCK, &mask, NULL);
        } else {
            addjob(jobs, pid, FG, cmdline);
            sigprocmask(SIG_UNBLOCK, &mask, NULL);    
            waitfg(pid);
        }
    }

    return;
}


/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 * Return 1 if a builtin command was executed; return 0
 * if the argument passed in is *not* a builtin command.
 */
int builtin_cmd(char **argv) 
{
    char *cmd = argv[0];
    if(!strcmp(cmd, "quit")) {
        exit(1);
    }
    if(!strcmp(cmd, "jobs")) {
        listjobs(jobs); 
        return 1;
    }
    if(!strcmp(cmd, "bg") || !strcmp(cmd, "fg")) {
        do_bgfg(argv);
        return 1;
    }
    if(!strcmp(cmd, "&")) {
        return 1;
    }

    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    char *cmd = argv[0];
    char *id = argv[1];

    if(id == NULL) {
        printf("%s command requires PID or %%jobid argument\n", cmd);
        return;
    }

    int jid;
    if(*id == '%') {
        jid = atoi(id + 1);
        if(jid == 0) {
            printf("%s: argument must be a PID or %%jobid\n", cmd);
            return;
        }
    } else {
        pid_t pid = atoi(id);
        if(pid == 0) {
            printf("%s: argument must be a PID or %%jobid\n", cmd);
            return;
        }
        jid = pid2jid(jobs, pid);
        if(jid == 0) {
            printf("(%d): No such process\n", pid);
            return;
        }
    }

    struct job_t *job = getjobjid(jobs, jid);
    if(job == NULL) {
        printf("%%%d: No such job\n", jid);
        return;
    }
     
    int job_state = job->state;
    if(!strcmp(cmd, "fg")) {
        job->state = FG;
        if(job_state == ST) {
            kill(-job->pid, SIGCONT);
        }
        waitfg(job->pid);
    } else { // bg
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);        
        job->state = BG;
        if(job_state == ST) {
            kill(-job->pid, SIGCONT);
        }
    }

    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    // int status;
    // waitpid(pid, &status, 0);
    // handle_child_returned(pid, status);

    sigset_t mask, contramask;
    sigemptyset(&mask);
    sigfillset(&contramask);
    sigaddset(&mask, SIGCHLD);
    sigdelset(&contramask, SIGCHLD);
    sigdelset(&contramask, SIGINT);
    sigdelset(&contramask, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    struct job_t *fg_job = getjobpid(jobs, pid);
    while(fg_job != NULL && fg_job->state == FG) {
        // printjob(fg_job);
        sigsuspend(&contramask);
    }

    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    return;
}

/*****************
 * Helper functions
 ****************/

/*
 * Handles child status processing after attempting reaping
 * with wait()
 */
// void handle_child_returned(pid_t pid, int status)
// {
//     if(WIFEXITED(status)) {
//         // int exitstatus = WEXITSTATUS(status);
//         deletejob(jobs, pid);        
//     } else if(WIFSIGNALED(status)) {
//         int jobid = pid2jid(jobs, pid);
//         printf("Job [%d] (%d) terminated by signal %d\n", jobid, pid, WTERMSIG(status));
//         deletejob(jobs, pid);
//     } else if(WIFSTOPPED(status)) {
//         struct job_t *job = getjobpid(jobs, pid);
//         job->state = ST;
//         printf("Job [%d] (%d) stopped by signal %d\n", job->jid, pid, WSTOPSIG(status));        
//     } else {
//         unix_error("Child did not exit successfully");
//     }
// }


/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int status;
    pid_t pid;
    while((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0) {
        if(WIFEXITED(status)) {
            // int exitstatus = WEXITSTATUS(status);
            deletejob(jobs, pid);        
        } else if(WIFSIGNALED(status)) {
            int jobid = pid2jid(jobs, pid);
            printf("Job [%d] (%d) terminated by signal %d\n", jobid, pid, WTERMSIG(status));
            deletejob(jobs, pid);
        } else if(WIFSTOPPED(status)) {
            struct job_t *job = getjobpid(jobs, pid);
            job->state = ST;
            printf("Job [%d] (%d) stopped by signal %d\n", job->jid, pid, WSTOPSIG(status));        
        } else {
            unix_error("Child did not exit successfully");
        }
    }
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    pid_t fg_pid = fgpid(jobs);
    if(fg_pid != 0) { // fg process exists
        kill(-fg_pid, SIGINT);
    }
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    pid_t fg_pid = fgpid(jobs);
    if(fg_pid != 0) { // fg process exists
        kill(-fg_pid, SIGTSTP);
    }
    return;
}

/*********************
 * End signal handlers
 *********************/



/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    ssize_t bytes;
    const int STDOUT = 1;
    bytes = write(STDOUT, "Terminating after receipt of SIGQUIT signal\n", 45);
    if(bytes != 45)
       exit(-999);
    exit(1);
}



