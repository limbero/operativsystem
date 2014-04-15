#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <unistd.h> /* definierar bland annat pipe() */
#include <sys/wait.h> /* definierar bland annat WIFEXITED */

#define READ 0
#define WRITE 1

void exit_with_error();

typedef struct Process {
	int pipe_fd[2];
	char *name;
} Process;

int main(int argc, char **argv, char **envp)
{
	int pipe_filedesc[2]; //Pipe for communication
	pid_t child_pid;
	int return_value;
	int num_proc = 3;

	if (argc > 1) {
		num_proc = 4;
	}
	Process processes[num_proc];

	int i;
	for (i = 0; i < num_proc; i++) {
		return_value = pipe(processes[i].pipe_fd)
	}


	return_value = pipe( pipe_filedesc );
	if (return_value == -1) {
		exit_with_error();
	}

	child_pid = fork();

	if (child_pid == 0) { //Pager child
		return_value = dup2( pipe_filedesc[ READ ], 0 ); /* STDIN_FILENO == 0 */
		if (return_value == -1) {
			exit_with_error();
		}
		return_value = close(pipe_filedesc[WRITE]);//Close write pipe
		if (return_value == -1) {
			exit_with_error();
		}
		return_value = close(pipe_filedesc[READ]);//Close read pipe
		if (return_value == -1) {
			exit_with_error();
		}

		char* const argv[2] = {"less", 0};

		execvp("less", argv);

		perror("Exec failed");
		exit(-1);

	} else { //Parent
		if (child_pid == -1) {
			exit_with_error();
		}
		return_value = dup2( pipe_filedesc[ WRITE ], 1 );
		if (return_value == -1) {
			exit_with_error();
		}
		return_value = close(pipe_filedesc[READ]);//Close read pipe
		if (return_value == -1) {
			exit_with_error();
		}
		return_value = close(pipe_filedesc[WRITE]);//Close read pipe
		if (return_value == -1) {
			exit_with_error();
		}
		wait(0);
	}

	return 0;
}

void exit_with_error() {
	perror("Error");
	exit(-1);
}