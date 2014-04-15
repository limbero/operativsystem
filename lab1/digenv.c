#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <unistd.h> /* definierar bland annat pipe() */
#include <sys/wait.h> /* definierar bland annat WIFEXITED */
#include <string.h> /* for strcmp */

/*
This program is an improved version of printenv.
It fetches all environment variables, sorts them
alphabetically and then prints them using a pager. 
Optionally, standard grep parameters can be used 
to only show specific variables, e.g.
> digenv HOME
to show variables that contain the string HOME
*/

#define READ 0 /* Index of read end of pipes */
#define WRITE 1 /* Index of write end of pipes */

void exit_with_error();
void close_pipes();

/* Struct for holding data for processes */
typedef struct Process {
	int pipe_fd[2]; /* The pipe between this and the next process */
	int useargv; /* If the process should use the program arguments or not */
	char *command[]; /* The name of the command, as an array */
} Process;

Process *processes[4]; /* array for holding the process data */

Process printenv = {{0,0}, 0, {"printenv", NULL}};
Process grep = {{0,0}, 1, {"grep", NULL}};
Process sort = {{0,0}, 0, {"sort", NULL}};
Process less = {{0,0}, 0, {"less", NULL}};

pid_t child_pid; /* Id for child processes */
int return_value; /* Placeholder for return values */
int num_proc = 3; /* How many processes to run (printenv, [grep], sort, less) */

int main(int argc, char **argv, char **envp)
{
	/* If there are program arguments, grep should be used */
	if (argc > 1) {
		num_proc = 4;
	}

	/* If there is a default pager, use that instead of less */
	char *pager = getenv("PAGER");
	if (pager != NULL) {
		less.command[0] = pager;
	}


	/* Add processes to array */
	int inc = 0;
	processes[inc++] = &printenv;
	if(num_proc == 4) /* Only use grep if there are arguments */
		processes[inc++] = &grep;
	processes[inc++] = &sort;
	processes[inc++] = &less;

	/* Create all pipes */
	int i;
	for (i=0; i < num_proc; i++) {
		if (pipe(processes[i]->pipe_fd) < 0) {
			exit_with_error();
		}
	}
	/* Begin the pipe chain */
	for (i=0; i < num_proc; i++) {

		child_pid = fork();

		if (child_pid == -1) { /* Parent process, fork failed */
			exit_with_error();

		} else if (child_pid == 0) { /* Child process */

			if (i != 0) { /* First step (printenv) does not nead read pipe */
				/* Switch stdin to the previous pipe */
				if (dup2(processes[i-1]->pipe_fd[READ], STDIN_FILENO ) < 0){
					exit_with_error();
				}
			}
			if (i != num_proc-1) { /* Last step should write to stdout */
				/* switch stdout to our pipe */
				if (dup2(processes[i]->pipe_fd[WRITE], STDOUT_FILENO ) < 0) {
					exit_with_error();
				}
			}

			/* Close all pipes, we just want streams 0 and 1 from now on */
			close_pipes();

			/* Execute the command */
			if (processes[i]->useargv) {
				argv[0] = processes[i]->command[0];
				execvp(processes[i]->command[0], argv);
			} else {
				return_value = execvp(processes[i]->command[0], processes[i]->command);
				/* If less does not exist, try more */
				if (return_value != 0 && strcmp(processes[i]->command[0], "less") == 0) {
					processes[i]->command[0] = "more";
					execvp(processes[i]->command[0], processes[i]->command);
				}
			}

			/* This code is only reached if there is an error in execvp */
			perror("Exec failed");
			exit(-1);

		} 
		
	}

	/* Parent does not use the pipe, so close all */
	close_pipes();
	
	/* Wait for children to finish */
	int j;
	for(j=0; j<num_proc; j++)
		wait(0);

	return 0;
}

/* Closes both ends of all initialized pipes */
void close_pipes() {
	int i;
	for (i=0; i<num_proc; i++) {
		return_value = close(processes[i]->pipe_fd[READ]); /* Close read pipe */
		if (return_value == -1) {
			exit_with_error();
		}
		return_value = close(processes[i]->pipe_fd[WRITE]); /* Close write pipe */
		if (return_value == -1) {
			exit_with_error();
		}
	}
}

/* Prints the last occurring error and exits the program with an error code */
void exit_with_error() {
	perror("Error");
	exit(-1);
}