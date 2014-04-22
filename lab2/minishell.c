#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <string.h> /* for strcmp */
#include <ctype.h> /* isspace */

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


		}
		//print info about finished command
		//check if background processes have finished
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