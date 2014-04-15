#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <unistd.h> /* definierar bland annat pipe() */
#include <sys/wait.h> /* definierar bland annat WIFEXITED */

#define READ 0
#define WRITE 1

void exit_with_error();
void close_pipes();

typedef struct Process {
	int pipe_fd[2];
	int useargv;
	char *command[];
} Process;

Process *processes[4];

Process printenv = {{0,0}, 0, {"printenv", NULL}};
Process grep = {{0,0}, 1, {"grep", NULL}};
Process sort = {{0,0}, 0, {"sort", NULL}};
Process less = {{0,0}, 0, {"less", NULL}};

int pipe_filedesc[2]; //Pipe for communication
pid_t child_pid;
int return_value;
int num_proc = 3;

int main(int argc, char **argv, char **envp)
{
	if (argc > 1) {
		num_proc = 4;
	}

	int inc = 0;

	processes[inc++] = &printenv;
	if(num_proc == 4)
		processes[inc++] = &grep;
	processes[inc++] = &sort;
	processes[inc++] = &less;

	int i;
	for (i=0; i < num_proc; i++) {
		return_value = pipe(processes[i]->pipe_fd);
		if (return_value == -1) {
			exit_with_error();
		}

		child_pid = fork();

		if (child_pid == 0) { //Pager child
			if (i != 0) {
				return_value = dup2(processes[i-1]->pipe_fd[READ], 0 ); //STDIN_FILENO == 0
				if (return_value == -1) {
					exit_with_error();
				}
			}
			if (i != num_proc-1) {
				return_value = dup2(processes[i]->pipe_fd[WRITE], 1 ); //STDOUT_FILENO == 1
				if (return_value == -1) {
					exit_with_error();
				}
			}

			close_pipes();

			if(processes[i]->useargv) {
				argv[0] = processes[i]->command[0];
				execvp(processes[i]->command[0], argv);
			}
			else {
				execvp(processes[i]->command[0], processes[i]->command);
			}

			perror("Exec failed");
			exit(-1);

		} else { //Parent
			if (child_pid == -1) {
				exit_with_error();
			}

			close_pipes();
		}
	}
	
	int j;
	for(j=0; j<num_proc; j++)
		wait(0);

	return 0;
}

void close_pipes() {
	int i;
	for (i=0; i<num_proc; i++) {
		return_value = close(processes[i]->pipe_fd[READ]); //Close read pipe
		if (return_value == -1) {
			exit_with_error();
		}
		return_value = close(processes[i]->pipe_fd[WRITE]); //Close write pipe
		if (return_value == -1) {
			exit_with_error();
		}
	}
}

void exit_with_error() {
	perror("Error");
	exit(-1);
}