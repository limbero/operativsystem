#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <string.h> /* for strcmp */
#include <ctype.h> /* isspace */
#include <sys/wait.h> /* definierar bland annat WIFEXITED */
#include <unistd.h> /* definierar bland annat pipe() */
#include <time.h> /* Time measurement */

/* Define booleans */
typedef int bool;
#define true 1
#define false 0

void clear_last_command();
void print_command_prompt(); /* Prints current directory and prompt sign */
void read_command(); /* Read input from stdin */
void parse_command(); /* Parse input string into array */
void print_finished_message(); /* Prints a message saying that the last foreground process has finished */

char command[100]; /* The command string that the user inputs */
bool background = false; /* Whether the current command should be run in background or not */
char* args[10]; /* The command arguments as an array */
int argc = 0; /* The number of arguments to the command */
pid_t child_pid; /* The process id of the last spawned process */
struct timeval start, end; /* Start and end time of process */

int main(int argcount, char **argv, char **envp)
{
	while ( true ) {
		clear_last_command();
		print_command_prompt();
		read_command();
		parse_command();
		if ( argc == 0 )
			continue;

		gettimeofday(&start, NULL);

		if ( strcmp(args[0], "exit") == 0 ) { /* if command is exit, exit */
			exit(0);

		} else if ( strcmp(args[0], "cd") == 0 ) {
			int return_value = chdir( args[1] ); /* Change directory */
			if (return_value == -1) { /* Error changing directory */
				char *home = getenv( "HOME" );
				if (home == NULL)
					home = "/";
				chdir( home );
			}
			
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

			/* Print spawned message */
			char *fgbg = (background) ? "background" : "foreground";
			printf("Spawned %s process %d (%s)\n", fgbg, child_pid, args[0]);

			/* Wait for process if it is not background */
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

		print_finished_message();

		/* TODO check if background processes have finished */
	}
}

void clear_last_command() {
	int i;
	for (i = 0; i < sizeof(command); i++)
		command[i] = '\0';
	for (i = 0; i < sizeof(args); i++)
		args[i] = '\0';
}

void print_command_prompt() {
	char current_dir[1024];
   	getcwd( current_dir, sizeof(current_dir) );
	printf( "minishell:%s$ ", current_dir );
}

void read_command() {
	char *buf = command;
	size_t size = sizeof( command );
	getline( &buf, &size, stdin );
}

void parse_command() {
	char * delimiter = " \n"; /* Split command string on space and newline */
	char *token = strtok( command, delimiter );
	int i = 0;
	while ( token != NULL ) {
		args[i++] = token;
		token = strtok( NULL, delimiter );
	}

	argc = i;
	background = false;
	if ( argc > 0 && strcmp( args[i-1], "&" ) == 0 ) {
		argc = i - 1;
		background = true;
	}
		
}

void print_finished_message() {
	if ( !background ) {
		gettimeofday(&end, NULL);
		long runtime = (long) (end.tv_sec - start.tv_sec) * 1000;
		runtime += (long) (end.tv_usec - start.tv_usec) / 1000;
		printf("Done: process %d (%s) ran for %ldms\n", child_pid, args[0], runtime);
	}
}