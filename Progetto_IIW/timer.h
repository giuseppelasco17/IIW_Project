#ifndef TIMER_H
#define TIMER_H


#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits.h>
#include <math.h>



#include "structure.h"
#include "linked_list.h"

/*Initialize the first timer in the timers' linked list*/
struct timer** initialize();

/*Create and initialize the timer*/
struct timer* make_timer(struct protocolSR* protocol, struct packet* pkt, timer_t* id_timer);

/*handles the expired timer event.
 * In particular the timer of the associated package restarts */
void timer_handler(union sigval);

/*Starts the timer*/
void create_timer(struct protocolSR* protocol, struct packet* pkt);

/*Delete the timer in the timers' linked list*/
void delete_timer(struct timer** first_timer, int seq);

/*trasform a value in nano second in a value in second and nano second*/
struct itimerspec timer_value_parser(long);

/*Returns 1 if the packet is lost, 0 otherwise.
 * the parameter is the probability of loss*/
int get_loss_probability(double *);

/*Calculates next time out interval*/
void to_calculate(struct protocolSR*, int);



#endif
