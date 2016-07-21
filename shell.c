#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "util.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>


/*
 * Do stuff with the shell input here.
 */
int sh_handle_input(char *line)
{
	//printf("x%sv\n", line);
	if (strlen(line) < 2)
	{
		return 0;
	}
	else if (strncmp(line,"\\seg", strlen("\\seg")) == 0)
	{
		/* Check for \seg command and create segfault */
		printf("Generating segfault...\n");
		char *n = NULL;
		*n = 1;
		//int i = *(int*)0;
	}
	return 1;
}
/*
 * Start the main shell loop:
 * Print, read user input, handle it.
 */
/*void sh_start(char *name, int fd_toserver)
{
	/***** Insert YOUR code *******/
//}
int main(int argc, char **argv)
{
	user_chat_box_t use;
	strcpy(use.name, argv[1]);
	/* Extract pipe descriptors and name from argv */
	//converting pipe file descriptors back to integers
	int rd = atoi(argv[2]);
	int wr = atoi(argv[3]);
	//int rd = (int) strtol(argv[2], (char **)NULL, 10);
	//int wr = (int) strtol(argv[3], (char **)NULL, 10);
	//printf("rd %d    wr %d\n", rd, wr);
	//forking a child process to write commands to server
	use.child_pid = fork();

	if (use.child_pid == -1)
	{
		exit(-1);
	}
	//Child process writes commands to write end of pipe
	else if (use.child_pid == 0)
	{
		//closing write end of pipe
		close(wr);
		char line[MSG_SIZE];
		fcntl(rd, F_SETFL, O_NONBLOCK);
		int nread;
		while(1)
		{
			nread = read(rd, line, MSG_SIZE);
			//printf("%d\n", nread);
			usleep(1000);
			if (nread > 0)
			{
				//printf("%d\n", nread);
				printf("%s\n", line);
				print_prompt(use.name);
			}
		}

		
	}
	//Parent process checks if there is anything was written from the server available to read.
	else
	{
		//closing read end of pipe
		close(rd);
		char l[50];
		char *line;
		char chpd[50] = "\\child_pid \0";
		int nwrite;
		//writing child_pid command in order to feed the child_pid back to server
		sprintf(l, "%d", getpid());
		strcat(chpd, l);
		write(wr, chpd, MSG_SIZE);
		print_prompt(use.name);
		while(1)
		{
			usleep(1000);
			line = sh_read_line();
			if (strlen(line) > 0)
			{
				if (sh_handle_input(line) == 1)
				{
					nwrite = write(wr, line, MSG_SIZE);
					if (nwrite < 0)
					{
						printf("*** ERROR: Failed to write to pipe\n");
            exit(1);
					}
					//close(wr);
				}
				else print_prompt(use.name);
				line[0] = '\0';
			}
		}	
	}
}
