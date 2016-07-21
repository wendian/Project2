#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include "util.h"

int userCount = 0;

/*
 * Identify the command used at the shell
 */




int main(int argc, char **argv)
{

	/***** Insert YOUR code *******/

	/* open non-blocking bi-directional pipes for communication with server shell */
	/* Fork the server shell */
/*

		maybe can use this to get the current directory
		char cwd[1024];
	 if (getcwd(cwd, sizeof(cwd)) != NULL){
		 printf(" %s\n", cwd);
	 }

*/

 char *temp = "/home/nguy1848/Desktop/Project2/chat/t.out";
 char *temp1 = "/home/nguy1848/Desktop/Project2/chat/XTERM";

	//execl(temp,temp, NULL);
//	printf("%s\n","I am here" );
	execl(temp1, "xterm", "+hold","-e",temp, char*(NULL));

	user_chat_box_t users[10];
	server_ctrl_t srv;
	pipe(srv.ptoc);
	pipe(srv.ctop);
	srv.pid = fork();


	if (srv.pid == -1)
	{
		printf("*** ERROR: forking child process failed\n");
    exit(1);
	}
	else if (srv.pid == 0)
	{
		srv.child_pid = (long)getpid();
		/*
		* Inside the child.
		* Start server's shell.
		* exec the SHELL program with the required program arguments.
		*/

		execl("CURR_DIR/SHELL_PROG", "./asdf","ser" srv.ptoc[0], srv.ctop[1], NULL);


		//char * shell_args[] = {"./shell", itoa(fd1), itoa(fd2), NULL};
	//	execvp("./shell", shell_args);
	}
	else
	{
		srv.pid = (long)getpid();
		close(srv.ptoc[0]);
		close(srv.ctop[1]);
		fcntl(srv.ctop[0], F_SETFL, O_NONBLOCK);
		/* Inside the parent. This will be the most important part of this program. */

		/* Start a loop which runs every 1000 usecs.
		* The loop should read messages from the server shell, parse them using the
		* parse_command() function and take the appropriate actions. */
		while (1)
			{
			/* Let the CPU breathe */
			usleep(1000);




			/*
			* 1. Read the message from server's shell, if any
			* 2. Parse the command
			* 3. Begin switch statement to identify command and take appropriate action
			*
			* 		List of commands to handle here:
			* 			CHILD_PID
			* 			LIST_USERS
			* 			ADD_USER
			* 			KICK  (kill(long)getpid(),9)
			* 			EXIT
			* 			BROADCAST
			*/



			/* Fork a process if a user was added (ADD_USER) */
			/* Inside the child */
			/*
			* Start an xterm with shell program running inside it.
			* execl(XTERM_PATH, XTERM, "+hold", "-e", <path for the SHELL program>, ..<rest of the arguments for the shell program>..);
			*/

			/* Back to our main while loop for the "parent" */
			/*
			* Now read messages from the user shells (ie. LOOP) if any, then:
			* 		1. Parse the command
			* 		2. Begin switch statement to identify command and take appropriate action
			*
			* 		List of commands to handle here:
			* 			CHILD_PID
			* 			LIST_USERS
			* 			P2P
			* 			EXIT
			* 			BROADCAST
			*
			* 		3. You may use the failure of pipe read command to check if the
			* 		user chat windows has been closed. (Remember waitpid with WNOHANG
			* 		from recitations?)
			* 		Cleanup user if the window is indeed closed.
			*/

	}
}	/* while loop ends when server shell sees the \exit command */

return 0;
}
