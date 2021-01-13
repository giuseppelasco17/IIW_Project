#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "structure.h"


/*Return the last timer in the linked list*/
struct timer* get_last_timer (struct timer** first_timer);

/*add an element in the linked list*/
int add_node(struct timer** first_timer, struct timer* new_timer) ;

int remove_node (struct timer* first_timer, struct timer* old_timer);

/*find an element in the linked list by sequence number*/
struct timer* find_node_by_seq(struct timer** first_timer, int seq);

/*removes an element from the linked list e deletes the associated timer*/
int remove_node_and_delete_timer_by_seq(struct timer** first_timer, int seq);

void list_timer(struct timer** first_timer);


#endif
