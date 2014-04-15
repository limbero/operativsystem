#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <unistd.h> /* definierar bland annat pipe() */
int main(int argc, char **argv, char **envp)
{
	int pipe_filedesc[2]; //Pipe for communication
	int child_pid;
	int return_value;

	return_value = pipe( pipe_filedesc );
	if (return_value == -1) {
		printf("pipe failure\n");
		exit(-1);
	}

	child_pid = fork();
	if (child_pid == -1) {
		printf("Fork failed\n");
		exit(-1);
	}

	if (child_pid == 0) { //Child
		return_value = close(pipe_filedesc[1]);//Close write pipe
		if (return_value == -1) {
			printf("Failed closing pipe\n");
			exit(-1);
		}
		int value;
		int bytes = read( pipe_filedesc[0], &value, sizeof(value) );
		if (bytes == -1) {
			printf("Failed reading\n");
			exit(-1);
		}
		printf("%d\n", value);
	} else { //Parent
		return_value = close(pipe_filedesc[0]);//Close write pipe
		if (return_value == -1) {
			printf("Failed closing pipe\n");
			exit(-1);
		}
		int value = 1;
		int bytes = write( pipe_filedesc[1], &value, sizeof(value) );
		if (bytes == -1) {
			printf("Failed writing\n");
			exit(-1);
		}
	}

	return 0;
}