CSci4061 S2016 Project 2

3/11/2016

Wendi An and Ly Nguyen

4638209 4551642

#server.c and shell.c

##1. Purpose
The purpose of this program is to make a chat application that implements pipe to send messages between users and a server that moderates the messages.

This is a program that is run from inside the terminal shell, in order to use it, you have to be running it from the UNIX shell.  You must also have XTerm installed to add users.

##2. Compile
To compile, use the Makefile included and simply type into the terminal:
```
make clean; make
```
##3. How to use from shell
To open the application from the shell, type:
```
./server
```
Do not attempt to run the shell executable by itself, it is meant only to be used by the server program.

##4. What the program does
From the server shell you have the following options:

| Server shell input        | Notes                                                          |
|---------------------------|----------------------------------------------------------------|
| \add \<username\>           | Opens an XTerm window that executes a shell for the new user   |
| \list                     | Prints a list of the names of all current users                |
| \kick \<username\>          | Terminates the XTerm of a user and removes user from list      |
| \exit                     | Terminates all active users and exits the program              |
| \<any-other-text\>          | Broadcasts the text to all active users                        |

From a user shell you have the following options:

| User shell input          | Notes                                                          |
|---------------------------|----------------------------------------------------------------|
| \p2p \<username\> \<message\> | Sends a private message to the specified user, fails if target does not exist |
| \list                     | Prints a list of the names of all current users for the requester |
| \exit                     | Terminates user's XTerm window and                             |
| \seg                      | Generates a segfault, the server will catch this and terminate the user's window |
| \<any-other-text\>          | Broadcasts the text to all active users                        |

##5. Assumptions
We assume that the user will not try to break the app by typing invalid commands such as ones that would cause the parsing methods to fail (for example "\addname"), since we used strtok, the returned string would be NULL and the new user's name might cause an error such as a segfault.

Also the user will not attempt to execute shell or rename any files.

##6. Error handling
Any system call is checked to see if it returns a -1, if it does, a proper error is printed.  If the error will cause the program to stop functioning properly, the entire process will exit.

________________
