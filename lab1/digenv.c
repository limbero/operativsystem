#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <unistd.h> /* definierar bland annat pipe() */
#include <sys/wait.h> /* definierar bland annat WIFEXITED */

#define READ 0
#define WRITE 1

void exit_with_error();

int main(int argc, char **argv, char **envp)
{
	int pipe_filedesc[2]; //Pipe for communication
	pid_t child_pid;
	int return_value;

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

		return_value = close(pipe_filedesc[READ]);//Close read pipe
		if (return_value == -1) {
			exit_with_error();
		}
		char* value = "hello";
		int bytes = write( pipe_filedesc[WRITE], value, sizeof("hello") - 1 );
		if (bytes == -1) {
			exit_with_error();;
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