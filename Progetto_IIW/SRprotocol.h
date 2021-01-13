#ifndef SRPROTOCOL_H
#define SRPROTOCOL_H

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
#include <stddef.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>

#include "structure.h"
#include "timer.h"


/*Initialize structure protocol*/
void initializeSR(struct protocolSR *protocol, int *sockfd1, struct sockaddr_in *addr1, int *sockfd2,
                  struct sockaddr_in *addr2, struct packet **window, int *win_size, pthread_mutex_t *global_lock,
                  struct timer **first_timer, double *loss_p, int* flag_timer, long* timer_value);

/*extracts the data from the recvData and place it in the ack structure*/
void extract_ack(char* recvData, size_t dimrecvData, struct ack* ack);

/*extracts the data from the buff recvData and place it in the pkt structure*/
void extract_pack(char* recvData, size_t dimrecvData, struct packet* pack);

/*makes the ack struct from the passed data*/
void make_ack(struct ack* ack, int ack_param, int seq);

/*makes the pkt struct from the passed data*/
void make_packet(struct packet* pack, int seq, int dimdata, const char* data, int ack);

/*it allows you to send data reliably by managing all the functions of the protocol window.
 * It competes with receiveSRack function.*/
void writeSR(int dim_file, int byte_read, struct packet* pack, struct protocolSR* protocol);

/*it allows you to receive ack reliably by managing all the functions of the protocol window.
 * It competes with writeSR function.*/
void receiveSRack(struct protocolSR* protocol );

/*Receives data from from the socket and pass it to the receive_routine function under form of struct pkt*/
void receiveSR(struct protocolSR* protocol);

/*it allows you to receive data reliably by managing all the functions of the protocol window.
 * Send ack for any allowable pkt and write it to a file in the right position*/
void receive_routine(struct protocolSR* protocol);

/*Write on file, specified in protocol, the data in the pack*/
void write_file(struct protocolSR* protocol, struct packet* pack);


#endif