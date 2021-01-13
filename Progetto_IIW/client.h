#ifndef CLIENT_H
#define CLIENT_H

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
#include <wait.h>

#include "SRprotocol.h"
#include "file_transfer.h"

/*manages the entire communication*/
void user_interface(void);

/*initializes the communication sockets between the client and server child process and sends the
 * command entered by the user*/
void initialize_socket_and_send_command();

/*calls the appropriate function based on the entered command*/
void command_handler_client();

/*initializes the connection socket of the parent process, requesting the port numbers to be used
 * for the communication sockets of the child process*/
void connection();

/*check the correctness of the parameters*/
void parser();

/*perform the steps necessary to fulfill the requested command*/
void list_client();
void put_client();
void get_client();



#endif
