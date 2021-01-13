#include <pthread.h>
#include "linked_list.h"


struct timer* get_last_timer (struct timer** first_timer){

    struct timer* last_timer;
    last_timer = *first_timer;

    while (last_timer->next_timer != NULL){
        last_timer = last_timer->next_timer;
    }

    return last_timer;

}


int add_node(struct timer** first_timer, struct timer* new_timer) {
    if (*first_timer == NULL){

        *first_timer = new_timer;
        (*first_timer)->next_timer = NULL;
        return 1;

    }else{
        new_timer->next_timer = NULL;
        struct timer* last_timer;
        last_timer = get_last_timer(first_timer);
        last_timer->next_timer = new_timer;
        return 2;
    }
}


int remove_node (struct timer* first_timer, struct timer* old_timer){

    struct timer* timer;
    struct timer* prev_timer;
    struct timer* next_timer;
    timer = first_timer;
    prev_timer = NULL;

    if (first_timer == NULL){
        return -1;
    }

    if ((first_timer->next_timer == NULL) && (first_timer == old_timer)){
        first_timer = NULL;
        return 0;
    }
    while (timer != NULL ){
        next_timer = timer->next_timer;
        if (timer == old_timer){

            prev_timer->next_timer = next_timer;
            free(old_timer->id_timer);
            free(old_timer);

            return 0;

        }
        prev_timer = timer;
        timer = timer->next_timer;

    }
    return -2;

}


struct timer* find_node_by_seq(struct timer** first_timer, int seq){

    struct  timer* timer = *first_timer;

    if (timer == NULL){
        return NULL;
    }
    while(timer != NULL ){
        if (timer->pkt->seq == seq){
            return timer;
        }
        timer = timer->next_timer;
    }
    return NULL;

}


int remove_node_and_delete_timer_by_seq(struct timer** first_timer, int seq){

    struct  timer* timer = *first_timer;
    struct timer* prev_timer = NULL;
    struct timer* next_timer;

    if (timer == NULL){
        return -1;
    }
    int count = 0;
    while(timer != NULL ){
        next_timer = timer->next_timer;
        if (timer->pkt->seq == seq){
            int ret;
            pthread_mutex_t* mutex = timer->mutex;
            ret = pthread_mutex_lock(mutex);
            if(ret != 0){
                perror("error in lock_mutex");
                exit(-1);
            }
            if (count == 0){
                (*first_timer) = timer->next_timer;
            }else{
                prev_timer->next_timer = next_timer;
            }

            timer->arrived = 1;
            ret = pthread_mutex_unlock(mutex);
            if(ret != 0){
                perror("error in unlock_mutex");
                exit(-1);
            }
            return 0;
        }

        prev_timer = timer;
        timer = timer->next_timer;
        count++;

    }

    return -2;
}






void list_timer(struct timer** first_timer){
    struct timer* timer;
    timer = *first_timer;
    while(timer != NULL){/*
        printf("Timer pointer = %p,\nPkt pointer = %p\nPkt seq = %d\nTimer id = %ld\n\n", timer,
               timer->pkt, timer->pkt->seq, (long)*(timer->id_timer)) ;*/
        timer = timer->next_timer;
    }
}