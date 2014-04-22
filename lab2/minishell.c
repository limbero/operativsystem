#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <string.h> /* for strcmp */
#include <ctype.h> /* isspace */
#include <sys/wait.h> /* definierar bland annat WIFEXITED */
#include <unistd.h> /* definierar bland annat pipe() */

/* Define booleans */
typedef int bool;
#define true 1
#define false 0

void read_command(); /* Read input from stdin */
void parse_command(); /* Parse input string into array */

char command_prompt[100]; /* The command string that the user inputs */
bool background = false; /* Whether the current command should be run in background or not */
char* args[10]; /* The command arguments as an array */
int argc = 0; /* The number of arguments to the command */
pid_t child_pid;

int main(int argc, char **argv, char **envp)
{
	while ( true ) {
		read_command( );
		parse_command( );

		if ( strcmp(args[0], "exit") == 0 ) { /* if command is exit, exit */
			exit(0);

		} else if ( strcmp(args[0], "cd") == 0 ) {
			/* TODO perform cd */
			
		} else {
			/* execute system command in child process */
			child_pid = fork();

			if (child_pid == -1) { /* Parent process, fork failed */
				exit(1); /* TODO print error message and continue? */

			} else if (child_pid == 0) { /* Child process */

				/* Execute the command */
				execvp(args[0], args);

				/* This code is only reached if there is an error in execvp */
				perror("Exec failed");
				exit(1); /* Should we kill the shell here, or just print an error and go on with our lives? */

			} 

			/* TODO Wait for process if it is not background */
			int status;
			if (waitpid(child_pid, &status, 0) == -1) {
				exit(1); /* TODO print error message and continue? */
			}
			if (WIFEXITED(status)) { 
				int child_status = WEXITSTATUS(status); /* Check which signal child process exited with */
				if (child_status != 0) { /* Something went wrong in child process */
					fprintf(stderr, "Child process (%s) failed with exit code %d\n", args[0], child_status);
				}
			} else if (WIFSIGNALED(status)) {
				int child_status = WTERMSIG(status); /* Child process ended by signal */
				fprintf(stderr, "Child process (%s) terminated by signal %d\n", args[0], child_status);
			}

		}
		/* TODO print info about finished command */
		/* TODO check if background processes have finished */
	}
}

void read_command() {
	char *buf = command_prompt;
	size_t size = sizeof( command_prompt );
	getline( &buf, &size, stdin );
}

void parse_command() {
	char * delimiter = " \n"; //Split command string on space and newline
	char *token = strtok( command_prompt, delimiter );
	int i = 0;
    while( token != NULL )
    {
    	args[i++] = token;
        token = strtok( NULL, delimiter );
    }
    argc = i;
	background = false;
    if ( strcmp( args[i-1], "&" ) == 0 ) {
    	argc = i - 1;
    	background = true;
    }
}