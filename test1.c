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

void cleanup_user(int idx, user_chat_box_t *users)
{
	if (idx == -1)
  {
    printf("User not found\n");
    return;
  }
	else
	{
		close_pipes(idx, users);
		kill(users[idx].child_pid,9);
		waitpid(users[idx].child_pid);
		kill(users[idx].pid,9);
		waitpid(users[idx].pid);
		users[idx].status = SLOT_EMPTY;
    userCount--;
		return;
	}
	/*
	 * Cleanup single user: close all pipes, kill user's child process, kill user
	 * xterm process, free-up slot.
	 * Remember to wait for the appropriate processes here!
	 */
}

void cleanup_users(user_chat_box_t *users)
{
	/*
	 * Cleanup all users: given to you
	 */
	int i;
	for (i = 0; i < MAX_USERS; i++) {
		if (users[i].status == SLOT_EMPTY)
			continue;
		cleanup_user(i, users);
	}
}

/*
 * Cleanup server process: close all pipes, kill the parent process and its
 * children.
 * Remember to wait for the appropriate processes here!
 */
void cleanup_server(server_ctrl_t srv)
{
	close(srv.ptoc[1]);
	close(srv.ctop[0]);
	kill(srv.child_pid,9);
	waitpid(srv.child_pid);
	//kill(srv.pid,9);
	//waitpid(srv.pid);
	return;
}

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
  sprintf(text, "%s : %s", sender, s);

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
 /*
  * Add a new user
  */
  /* Fork a process if a user was added (ADD_USER) */
  /* Inside the child */
  /*
  * Start an xterm with shell program running inside it.
  * execl(XTERM_PATH, XTERM, "+hold", "-e", <path for the SHELL program>, ..<rest of the arguments for the shell program>..);
  */
	if(userCount == MAX_USERS)
  {
		printf("%s\n","Could not add user: Resource temporarily unavailable");
	}
	else
  {
    buf = strtok(buf, "\n");
		userCount++;
    int place;
    place = find_empty(users);
    strcpy(users[place].name,buf);
    users[place].status = SLOT_FULL;
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
    else if(users[place].pid == 0)
    {
      printf("Adding user %s...\n", users[place].name);
      char fd0[25];
      char fd1[25];
      sprintf(fd0, "%d", users[place].ptoc[0]);
      sprintf(fd1, "%d", users[place].ctop[1]);
      execl(XTERM_PATH,XTERM_PATH,"-hold","-e", "./shell", users[place].name, fd0, fd1, NULL);
    }
    close(users[place].ptoc[0]);
    close(users[place].ctop[1]);
		char val[1024];
		val[0] = '\0';

		//while(strlen(val) < 1) read(users[place].ctop[0], val, 1024);
    read(users[place].ctop[0], val, 1024);
		users[place].child_pid = (int) strtol(val, (char **)NULL, 10);
    //printf("userpid %d  input %s\n", users[place].child_pid, val);
    fcntl(users[place].ctop[0], F_SETFL, O_NONBLOCK);
    return 0;
  }

 	/*
 	 * Check if user limit reached.
 	 *
 	 * If limit is okay, add user, set up non-blocking pipes and
 	 * notify on server shell
 	 *
 	 * NOtE: You may want to remove any newline characters from the name string
 	 * before adding it. This will help in future name-based search.
 	 */
 }

char *extract_name(int cmd, char *buf)
{
	char *s = NULL;

	s = strtok(buf, " ");
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
		 int i;
   char uname[1024];
   char list[2048];
   list[0] = '\0';
		 for(i = 0; i < MAX_USERS; i++)
     {
       //printf("%d\n", i);
       if (users[i].status == SLOT_FULL)
       {
         strcpy(uname, users[i].name);
         strcat(uname, "\n");
         strcat(list,uname);
       }
		 }
   write(fd, list, MSG_SIZE);
	 }
}

void intitialize(user_chat_box_t *users)
{
  int i;
  for (i = 0; i < MAX_USERS; i++)
  {
    users[i].status = SLOT_EMPTY;
  }
}

int find_empty(user_chat_box_t *users)
{
  int i;
  for (i = 0; i < MAX_USERS; i++)
  {
    if (users[i].status == SLOT_EMPTY) return i;
  }
  return -1;
}

void send_p2p_msg(int sender, int target, user_chat_box_t *users, char *buf)
{
  /*
   * Send personal message. Print error on the user shell if user not found.
	get the target user by name (hint: call (extract_name() and send message */
  if (target == -1)
  {
    printf("User not found\n");
    return;
  }
  else
  {
    const char *s;
    char sender_name[1024];
    strcpy(sender_name,users[sender].name);
    char text[MSG_SIZE];
    s = strtok(buf, " ");
    s = strtok(NULL, " ");
    s = strtok(NULL, "\n");
    sprintf(text, "%s : %s", sender_name, s);
    if (write(users[target].ptoc[1], text, strlen(text) + 1) < 0)
    {
    		perror("write to child shell failed");
    }
    return;
  }
}


int main(int argc, char **argv)
{
  // This is the one we should  execl("/usr/bin/xterm","usr/bin/xterm","-hold","-e", "./shell", '\0');
  //	execl("XTERM_PATH", "xterm", "+hold","-e", "t.out", '\0');
  //array to hold all the people
  //main server shell
  user_chat_box_t users[MAX_USERS];
  intitialize(users);
  server_ctrl_t srv;
  //open server shell pipes
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
  srv.pid = fork();

  if (srv.pid == -1)
  {
    printf("*** ERROR: forking child process failed\n");
    exit(1);
  }
  else if (srv.pid == 0)
  {
  //srv.child_pid = (long)getpid();
  /*
  * Inside the child.
  * Start server's shell.
  * exec the SHELL program with the required program arguments.
  */
  char fd0[50];
  char fd1[50];
  sprintf(fd0, "%d", srv.ptoc[0]);
  sprintf(fd1, "%d", srv.ctop[1]);
  //printf("pipe 1:  %d\n", srv.ctop[1]);
  //printf("fd 1:  %s\n",fd1);
  //printf("%s, %s\n", fd0, fd1);
  execl("./shell", "./shell", "Server", fd0, fd1, NULL);

  //char * shell_args[] = {"./shell", itoa(fd1), itoa(fd2), NULL};
  //	execvp("./shell", shell_args);
  }
  else
  {
    srv.pid = (long)getpid();
    close(srv.ptoc[0]);
    close(srv.ctop[1]);
    fcntl(srv.ctop[0], F_SETFL, O_NONBLOCK);
    /*
    /* Inside the parent. This will be the most important part of this program. */
    /* Start a loop which runs every 1000 usecs.
    * The loop should read messages from the server shell, parse them using the
    * parse_command() function and take the appropriate actions. */
    server_cmd_type command;
    char line[MSG_SIZE];
    int  i,target=-1, closer = 0;
    char *content;
    char reqname[1024];
    int req_no,nread;
    char line1[MSG_SIZE];
		line[0] = '\0';
		read(srv.ctop[0], line, MSG_SIZE);
		srv.child_pid = (int) strtol(line, (char **)NULL, 10);
    while (1)
    {
      /* Let the CPU breathe */
      usleep(100000);

      //sss = scanf("%s\n", &line);
      //printf("%d\n", sss);
      nread = read(srv.ctop[0], line, MSG_SIZE);
			//printf("%d\n", nread);
      if (strlen(line) > 1)
      {
        strcpy(reqname, "Server");
        command = parse_command(line);
        switch(command)
    		{
    			case CHILD_PID:
    				break;
    			case LIST_USERS:
            list_users(users, srv.ptoc[1]);
    				break;
    			case ADD_USER:
            content = extract_name(command, line);
            add_user(users, content);
						content = '\0';
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
						content = '\0';
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
        reqname[0] = '\0';
        command = BROADCAST;
        line[0] = '\0';
      }
      for (i = 0; i < MAX_USERS; i++)
      {
        if (users[i].status == SLOT_FULL)
        {
          nread = read(users[i].ctop[0], line, MSG_SIZE);
					//printf("2nd %d\n", nread);
          if (strlen(line) > 1)
          {
            strcpy(reqname, users[i].name);
            req_no = users[i].ptoc[1];
            command = parse_command(line);
            switch(command)
        		{
        			case CHILD_PID:
        				break;
        			case P2P:
                strcpy(line1, line);
                content = extract_name(command, line);
                target = find_user_index(users, content);
                send_p2p_msg(i, target, users, line1);
                line1[0] = '\0';
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
            req_no = 0;
            reqname[0] = '\0';
            command = BROADCAST;
            content = '\0';
            line[0] = '\0';
            target = -1;
						if (closer == 1)
						{
							break;
						}
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
    }
  }	/* while loop ends when server shell sees the \exit command */
  return 0;
}
