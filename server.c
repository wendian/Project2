#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include "util.h"

int userCount = 0;

int parse_command(char *buf)
{
 /*
  * Identify the command used at the shell
  */
	int cmd;

	if (starts_with(buf, CMD_CHILD_PID))
		cmd = CHILD_PID;
	else if (starts_with(buf, CMD_P2P))
		cmd = P2P;
	else if (starts_with(buf, CMD_LIST_USERS))
		cmd = LIST_USERS;
	else if (starts_with(buf, CMD_ADD_USER))
		cmd = ADD_USER;
	else if (starts_with(buf, CMD_EXIT))
		cmd = EXIT;
	else if (starts_with(buf, CMD_KICK))
		cmd = KICK;
	else
		cmd = BROADCAST;

	return cmd;
}

void close_pipes(int idx, user_chat_box_t *users)
{
	/*
	 * Close all pipes for this user
	 */
	 close(users[idx].ptoc[1]);
	 close(users[idx].ptoc[0]);
}

//this function closes the pipes, kills process, and its child process
void cleanup_user(int idx, user_chat_box_t *users)
{
	if (idx == -1)
  {
		//Use was not found
    printf("User not found\n");
    return;
  }
	else
	{
		//closing pipes of user we want to kick
    printf("child %d  parent %d\n", users[idx].child_pid, users[idx].pid );
		int status;
		close_pipes(idx, users);
		//killing the user's child processes
		kill(users[idx].child_pid,9);
		//waitpid(users[idx].child_pid, &status, WNOHANG);
		//killing the user process
		kill(users[idx].pid,9);
		wait(NULL);
    //waitpid(users[idx].pid);
		//waitpid(NULL);
		//updating the users array and userCount
		users[idx].status = SLOT_EMPTY;
    userCount--;
		return;
	}
}

//cleanup for all users
void cleanup_users(user_chat_box_t *users)
{
	//cleaning up all users
	int i;
	for (i = 0; i < MAX_USERS; i++) {
		if (users[i].status == SLOT_EMPTY)
			continue;
		cleanup_user(i, users);
	}
}

void cleanup_server(server_ctrl_t srv)
{
	//closing server's pipes, killing process and child process
  //printf("child %d  parent %d\n",srv.child_pid, srv.pid );
	close(srv.ptoc[1]);
	close(srv.ctop[0]);
  //printf("killing %d\n", srv.child_pid);
	kill(srv.child_pid,9);
	kill(srv.pid,9);
	wait(NULL);
	return;
}

//this function is used to broadcast messages to the users
int broadcast_msg(user_chat_box_t *users, char *buf, int fd, char *sender)
{
  int i;
  const char *msg = "Broadcasting...", *s;
  char text[MSG_SIZE];
  /* Notify on server shell */
  if (write(fd, msg, strlen(msg) + 1) < 0)
  	perror("writing to server shell");
  /* Send the message to all user shells */
  s = strtok(buf, "\n");

	//formats the beginning of the message
  sprintf(text, "%s : %s", sender, s);

	//iterates over the users array and writes message if SLOT_FULL
  for (i = 0; i < MAX_USERS; i++)
  {
  	if (users[i].status == SLOT_EMPTY)
    {
      continue;
    }
  	if (write(users[i].ptoc[1], text, strlen(text) + 1) < 0)
    {
    		perror("write to child shell failed");
    }
  }
}

//returns the user index if they exist in the user_chat_box_t
int find_user_index(user_chat_box_t *users, char *name)
{
  int i, user_idx = -1;
  if (name == NULL)
  {
    fprintf(stderr, "NULL name passed.\n");
    return user_idx;
  }
  for (i = 0; i < MAX_USERS; i++)
  {
    if (users[i].status == SLOT_EMPTY)
    continue;
		//a user was found with a matching name
    if (strncmp(users[i].name, name, strlen(name)) == 0)
    {
      user_idx = i;
      break;
    }
  }
  return user_idx;
}

int add_user(user_chat_box_t *users, char *buf)
{
	//UserCount is at maximum, user can not be added
	if(userCount == MAX_USERS)
  {
		printf("%s\n","Could not add user: Resource temporarily unavailable");
	}
	else
  {
    buf = strtok(buf, "\n");
		//increments userCount and finds a SLOT_EMPTY to place the new user
		userCount++;
    int place;
    place = find_empty(users);
    strcpy(users[place].name,buf);
    users[place].status = SLOT_FULL;
		//checking if creation of pipe failed
    if (pipe(users[place].ctop) == -1)
    {
      printf("*** ERROR: piping new child failed\n");
      exit(1);
    }
    if (pipe(users[place].ptoc) == -1)
    {
      printf("*** ERROR: piping new child failed\n");
      exit(1);
    }
    users[place].pid = fork();
    if (users[place].pid == -1)
    {
      printf("*** ERROR: forking child process failed\n");
      exit(1);
    }
		//Inside the child process, we execute an XTERM window and pass the pipe file descriptors to shell
    else if(users[place].pid == 0)
    {
      printf("Adding user %s...\n", users[place].name);
      char fd0[25];
      char fd1[25];
      sprintf(fd0, "%d", users[place].ptoc[0]);
      sprintf(fd1, "%d", users[place].ctop[1]);
      execl(XTERM_PATH,XTERM_PATH,"+hold","-e", "./shell", users[place].name, fd0, fd1, NULL);
    }
		//closing unneeded pipe ends
    close(users[place].ptoc[0]);
    close(users[place].ctop[1]);
		//setting pipe to non-blocking
    fcntl(users[place].ctop[0], F_SETFL, O_NONBLOCK);
    return 0;
  }
 }

char *extract_name(int cmd, char *buf)
{
	char *s = NULL;
	s = strtok(buf, " ");
	//extracts name from command
	s = strtok(NULL, " ");
	if (cmd == P2P)
		return s;	/* s points to the name as no newline after name in P2P */
	s = strtok(s, "\n");	/* other commands have newline after name, so remove it*/
	return s;
}

int list_users(user_chat_box_t *users, int fd)
{
  /*
   * List the existing users on the server shell
	 * Construct a list of user names
	 * Don't forget to send the list to the requester!
	 */
	 if(userCount == 0)
   {
		 printf("%s\n","<no users>");
	 }
	 else
   {
		//setting up variables to extract names of users
	 int i;
   char uname[1024];
   char list[2048];
   list[0] = '\0';
		 for(i = 0; i < MAX_USERS; i++)
     {
       //printf("%d\n", i);
			 //concatenating a string that lists all users
       if (users[i].status == SLOT_FULL)
       {
         strcpy(uname, users[i].name);
         strcat(uname, "\n");
         strcat(list,uname);
       }
		 }
		 //write the list in the pipe of the requester
   write(fd, list, MSG_SIZE);
	 }
}

//initializes all slots in users array to SLOT_EMPTY
void intitialize(user_chat_box_t *users)
{
  int i;
  for (i = 0; i < MAX_USERS; i++)
  {
    users[i].status = SLOT_EMPTY;
  }
}

//returns the index of an empty slot and -1 if there are none
int find_empty(user_chat_box_t *users)
{
  int i;
  for (i = 0; i < MAX_USERS; i++)
  {
    if (users[i].status == SLOT_EMPTY) return i;
  }
  return -1;
}


//function is used to send a private message from one user to another
void send_p2p_msg(int sender, int target, user_chat_box_t *users, char *buf)
{
  /*
   * Send personal message. Print error on the user shell if user not found.
	get the target user by name (hint: call (extract_name() and send message */

  char text[MSG_SIZE];
	//the reciever of the message does not exist
  if (target == -1)
  {
    strcpy(text, "User not found\n");
    write(users[sender].ptoc[1], text, strlen(text) + 1);
    return;
  }
  else
  {
    const char *s;
    char sender_name[1024];
    strcpy(sender_name,users[sender].name);
    s = strtok(buf, " ");
    s = strtok(NULL, " ");
    s = strtok(NULL, "\n");
		//formatting the message to show the sender and reciever of message
    sprintf(text, "%s : %s", sender_name, s);
    if (write(users[target].ptoc[1], text, strlen(text) + 1) < 0)
    {
    		printf("Write to child shell failed");
				exit(1);
    }
    return;
  }
}


int main(int argc, char **argv)
{
  //array to hold all the people, creating main user_chat_box_t and initializes all slots to empty
  user_chat_box_t users[MAX_USERS];
  intitialize(users);
	//struct to represent the server's info
  server_ctrl_t srv;
  //open server shell pipes and checking for failure
  if (pipe(srv.ptoc) < 0)
  {
    printf("*** ERROR: pipe process failed\n");
    exit(1);
  }
  if (pipe(srv.ctop) < 0)
  {
    printf("*** ERROR: pipe process failed\n");
    exit(1);
  }
	//forking for child process to execute the server shell and parent to communicate with users
  srv.pid = fork();

  if (srv.pid == -1)
  {
    printf("*** ERROR: forking child process failed\n");
    exit(1);
  }
  else if (srv.pid == 0)
  {
	//converting file descriptors to strings
  char fd0[50];
  char fd1[50];
  sprintf(fd0, "%d", srv.ptoc[0]);
  sprintf(fd1, "%d", srv.ctop[1]);
	//executing server shell
  execl("./shell", "./shell", "Server", fd0, fd1, NULL);
  }
  else
  {
    //srv.pid = (long)getpid();
		//closing unneeded pipes
    close(srv.ptoc[0]);
    close(srv.ctop[1]);
		//setting reading pipe to non-blocking
    fcntl(srv.ctop[0], F_SETFL, O_NONBLOCK);
    /* Inside the parent. This will be the most important part of this program. */
    /* Start a loop which runs every 1000 usecs.
    * The loop should read messages from the server shell, parse them using the
    * parse_command() function and take the appropriate actions. */
    server_cmd_type command;
    char line[MSG_SIZE];
    char line1[MSG_SIZE];
    char *content;
    char reqname[1024];
    int target=-1, closer = 0;
    int req_no, nread, i, status;
    //printf("%s\n", line);
		//srv.child_pid = (int) strtol(line, (char **)NULL, 10);
    while (1)
    {
      /* Let the CPU breathe */
      usleep(1000000);
      //sss = scanf("%s\n", &line);
      //printf("%d\n", sss);
      nread = read(srv.ctop[0], line, MSG_SIZE);
      //if (strlen(line) > 1)
      //if (nread == 0)
      if (nread > 0)
      {
        strcpy(reqname, "Server");
				//extracting command from line
        command = parse_command(line);
        switch(command)
    		{
    			case CHILD_PID:
						//setting the child_pid
            content = extract_name(command, line);
            //srv.child_pid = (long) strtol(content, (char **)NULL, 10);
						srv.child_pid = atoi(content);
            content  = '\0';
    				break;
    			case LIST_USERS:
            list_users(users, srv.ptoc[1]);
    				break;
    			case ADD_USER:
            content = extract_name(command, line);
            add_user(users, content);
    				break;
    			case EXIT:
						cleanup_users(users);
						cleanup_server(srv);
						closer = 1;
            break;
          case KICK:
						content = extract_name(command, line);
						target = find_user_index(users, content);
						cleanup_user(target, users);
						target = -1;
            break;
          case BROADCAST:
            broadcast_msg(users, line, srv.ptoc[1], reqname);
            break;
    			default:
            break;
            printf("*** ERROR: No command recognized\n");
            exit(1);
    		}
				//resetting variables
				content = '\0';
        reqname[0] = '\0';
        command = BROADCAST;
        line[0] = '\0';
      }
			//this loop iterates through all the users' pipes, checking if there is a command to be read
      for (i = 0; i < MAX_USERS; i++)
      {
        if (users[i].status == SLOT_FULL)
        {
          printf("pid : %d  \n  %d\n", users[i].pid, waitpid(users[i].pid, &status, WNOHANG));
          nread = read(users[i].ctop[0], line, MSG_SIZE);
          //status = 0;
          /*
          if (waitpid(users[i].child_pid, &status, WNOHANG) != 0)
          {
            cleanup_user(i, users);
          }
          */
					if (nread == 0) cleanup_user(i, users);
					//if (nread < 0) printf("%d   error :::  %s\n", nread, strerror(errno));
          if (nread > 1)
          {
						//saving the name of requesting user that had a command
            strcpy(reqname, users[i].name);
            req_no = users[i].ptoc[1];
            command = parse_command(line);
            switch(command)
        		{
        			case CHILD_PID:
                content = extract_name(command, line);
                //users[i].child_pid = (long) strtol(content, (char **)NULL, 10);
								users[i].child_pid = atoi(content);
        				break;
        			case P2P:
							//requester sending a private message to a user
                strcpy(line1, line);
                content = extract_name(command, line);
                target = find_user_index(users, content);
                send_p2p_msg(i, target, users, line1);
        				break;
        			case LIST_USERS:
                list_users(users, req_no);
        				break;
        			case EXIT:
                cleanup_user(i, users);
                break;
              case BROADCAST:
                broadcast_msg(users, line, srv.ptoc[1], reqname);
                break;
        			default:
                break;
                printf("*** ERROR: No command recognized\n");
                exit(1);
        		}
						//resetting variables
            req_no = 0;
            reqname[0] = '\0';
            command = BROADCAST;
            content = '\0';
            line[0] = '\0';
            target = -1;
          }
        }
      }
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
      if (closer == 1)
      {
        break;
      }
    }
  }	/* while loop ends when server shell sees the \exit command */
  return 0;
}
