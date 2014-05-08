#define _XOPEN_SOURCE 500 /* for sigset, aningens oklar */

#include <signal.h> /* for sigset */
#include <sys/types.h> /* definierar typen pid_t */
#include <errno.h> /* definierar bland annat perror() */
#include <stdio.h> /* definierar bland annat stderr */
#include <stdlib.h> /* definierar bland annat rand() och RAND_MAX */
#include <string.h> /* for strcmp */
#include <ctype.h> /* isspace */
#include <sys/wait.h> /* definierar bland annat WIFEXITED */
#include <unistd.h> /* definierar bland annat pipe() */
#include <sys/time.h> /* tidtagning */

/* definiera booleans */
typedef int bool;
#define true 1
#define false 0

void catch_function(int signo); /* vi vill inte gora nagonting, bara fanga signalen */
void clear_last_command(); /* tom det forra kommandot och dess argument */
void print_command_prompt(); /* skriv ut var prompt, innehallandes working directory */
void read_command(); /* las in nasta kommando fran stdin */
void parse_command(); /* gor om kommandot fran en ren strang till en array med kommando och argument */
void print_finished_message(); /* skriv ut kortid och vilken process det galler */
void check_bg_processes(); /* leta efter avslutade bakgrundsprocesser */

char command[100]; /* kommandot */
bool background = false; /* ar det en bakgrundsprocess eller ej, svara mig det */
char* args[10]; /* kommandots argument som array */
int argc = 0; /* antalet argument kommandot har */
pid_t child_pid; /* processid for senast skapade processen */
struct timeval start, end; /* start- och end-tid for processer */

int main(int argcount, char **argv, char **envp)
{
	if (sigset(SIGINT, catch_function) == SIG_ERR)
 	 	printf("Couldn't catch SIGINT\n");

 	/* Loopa forever */
	while ( true ) {
		clear_last_command(); /* se till att inget skrap ligger kvar fran forra kommandot */
		print_command_prompt(); /* skriv ut nuvarande katalog som prompt */
		read_command(); /* las anvandarens input */
		parse_command(); /* dela upp inputraden i delar och processa */
		if ( argc == 0 )
			continue;

		gettimeofday(&start, NULL); /* spara starttiden sa att vi kan mata exekveringstiden */

		if ( strcmp(args[0], "exit") == 0 ) { /* om kommandot ar exit, exita */
			exit(0);

		} else if ( strcmp(args[0], "cd") == 0 ) { /* anvand var implementation av cd */
			int return_value = chdir( args[1] ); /* byt working directory */
			if (return_value == -1) { /* att byta directory misslyckades */
				char *home = getenv( "HOME" );
				if (home == NULL)
					home = "/";
				chdir( home ); /* ga hem istallet */
			}
			
		} else {
			/* skapa ny barnprocess for systemkommandot */
			child_pid = fork();

			if (child_pid == -1) { /* foraldraprocessen, fork failade */
				exit(1); /* kill it with fire */

			} else if (child_pid == 0) { /* barnprocess */

				/* exekvera kommandot */
				execvp(args[0], args);

				/* hit kommer man bara om execvp failar */
				perror("Exec failed");
				exit(1); /* om exekveringen failar, doda denna kopia av shellet */

			} 

			/* skriv ut vilken process som skapats */
			char *fgbg = (background) ? "background" : "foreground";
			printf("Spawned %s process %d (%s)\n", fgbg, child_pid, args[0]);

			/* vanta pa forgrundsprocesser */
			if ( !background ) {

				int status;
				if (waitpid(child_pid, &status, 0) == -1) {
					/* gor inget om den inte finns */
				}
				if (WIFEXITED(status)) { /* barnprocessen avslutades */
					int child_status = WEXITSTATUS(status); /* kolla vilken signal barnprocessen avslutades med */
					if (child_status != 0) { /* barnprocessen har avslutat med felmeddelande */
						fprintf(stderr, "Child process (%s) failed with exit code %d\n", args[0], child_status);
					} else {
						print_finished_message();
					}
				} else if (WIFSIGNALED(status)) { /* barnprocessen har avslutats av en signal */
					int child_status = WTERMSIG(status);
					fprintf(stderr, "Child process (%s) terminated by signal %d\n", args[0], child_status);
					print_finished_message();
				}
				waitpid(child_pid, &status, 0); /* se till att barnet inte förblir zombie om den har avbrutits */

			}
		}

		check_bg_processes(); /* kolla om nagon bakgrundsprocess avslutats sedan senaste kommandot */
	}
}

void catch_function(int signo) {
	/* gor ingenting */
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
   	getcwd( current_dir, sizeof(current_dir) ); /* hamta nuvarande katalog */
	printf( "minishell:%s$ ", current_dir );
}

void read_command() {
	char *buf = command;
	size_t size = sizeof( command );
	while(fgets( buf, size, stdin ) == NULL && errno == EINTR ); /* las nasta rad tills fgets inte returnerar NULL och far interrupt error */
}

void parse_command() {
	char * delimiter = " \n"; /* dela upp kommandostrangen pa space och newline */
	char *token = strtok( command, delimiter );
	int i = 0;
	while ( token != NULL ) { /* tills det inte finns nagon delstrang kvar */
		args[i++] = token;
		token = strtok( NULL, delimiter ); /* hamta nasta delstrang */
	}

	argc = i;
	background = false;

	/* satt background till true om sista argumentet ar '&' */
	if ( argc > 0 && strcmp( args[i-1], "&" ) == 0 ) {
		argc = i - 1; /* rakna antalet argument som ett mindre */
		args[i-1] = '\0'; /* ta bort det sista argumentet, dvs '&' */
		background = true;
	}
		
}

void print_finished_message() {
	if ( !background ) {
		gettimeofday(&end, NULL); /* hamta nuvarande tid */
		long runtime = (long) (end.tv_sec - start.tv_sec) * 1000; /* runtime ar endtid minus starttid, for bade sekunddelen */
		runtime += (long) (end.tv_usec - start.tv_usec) / 1000; /* och mikrosekunddelen */
		printf("Done: foreground process %d (%s) has finished and ran for %ldms\n", child_pid, args[0], runtime);
	}
}

void check_bg_processes() {
	int status;
	pid_t pid;

	while ( true ) { /* loopa forever */
		pid = waitpid(-1, &status, WNOHANG);
		if (pid == 0 || pid == -1) /* detta betyder att det inte fanns nagon process kvar att titta pa */
			break;

		printf("Done: background process %d has finished\n", pid);
	}
}
