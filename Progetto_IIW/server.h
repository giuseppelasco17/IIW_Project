#ifndef SERVER_H
#define SERVER_H


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/sem.h>

#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "SRprotocol.h"
#include "timer.h"
#include "file_transfer.h"

/*calls the appropriate function based on the entered command*/
void command_handler(char *client_cmd, char* cmd_filename);

/*initializes the communication sockets between the client and server child process and sends the
 * command entered by the user*/
void initialize_transfer_socket();

/*initializes the connection socket of the parent process, sending the port numbers to be used
 * for the communication sockets of the child process*/
void connection();

/*perform the steps necessary to fulfill the requested command*/
void list();
void put();
void get();


#endif
