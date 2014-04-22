#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <string.h> /* for strcmp */

typedef int bool;
#define true 1
#define false 0

char* read_command();
void parse_command( char** args, bool* background, char* command_prompt);
bool built_in_command( char* command );

int main(int argc, char **argv, char **envp)
{
	while ( true ) {
		char* command_prompt = read_command();
		bool background;
		char* args[1000];
		parse_command( args, &background, command_prompt );
		if ( built_in_command(args[0]) ) {
			printf( "command: %s\n", args[0] );
			//perform cd or exit
			//if command is exit, exit
			if ( strcmp(args[0], "exit") == 0 )
				exit(0);
		} else {
			//execute system command in child process

		}
		//print info about finished command
		//check if background processes have finished
	}
}

char* read_command() {
	return "command args &";
}

void parse_command( char** args, bool* background, char* command_prompt ) {
	args[0] = "exit";
	*background = false;
}

bool built_in_command( char* command ) {
	if ( strcmp(command, "cd") == 0 || strcmp(command, "exit") == 0 )
		return true;
	return false;
}