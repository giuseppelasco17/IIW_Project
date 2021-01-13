#include <pthread.h>
#include "timer.h"


struct itimerspec timer_value_parser(long nano_sec){

    int sec = nano_sec/ 1000000000;
    long nano = nano_sec % 1000000000;

    struct itimerspec ts;
    ts.it_value.tv_sec = sec;
    ts.it_value.tv_nsec = nano;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;
    return ts;

}

struct timer** initialize(){

    struct timer** first_timer = (struct timer** ) malloc(sizeof(struct timer));
    struct timer* timer =(struct timer*) malloc(sizeof(struct timer));
    timer = NULL;
    (*first_timer) = timer;
    return  first_timer;
}


struct timer* make_timer(struct protocolSR* protocol, struct packet* pkt, timer_t* id_timer){

    struct timer* timer = malloc(sizeof(struct timer));
    pthread_mutex_t* mutex = malloc(sizeof(pthread_mutex_t));

    int ret = pthread_mutex_init(mutex, NULL);
    if(ret != 0) {
        perror("error in mutex_init");
        exit(-1);
    }

    memset(timer, 0, sizeof(struct timer));
    timer->id_timer = id_timer;
    timer->pkt = pkt;
    timer->protocol = protocol;
    timer->mutex = mutex;
    timer->arrived = 0;
    return timer;
}

void timer_handler(union sigval sv){

    struct timer* timer = sv.sival_ptr;
    int ret;
    ret = pthread_mutex_lock(timer->mutex);
    if(ret != 0){
        perror("errore in lock_mutex");
        exit(-1);
    }

    //already eliminated from linked-list
    if (timer->arrived == 1){

        ret = timer_delete(*(timer->id_timer));
        if(ret == -1){
            printf("ERRNO: %s\n",strerror(errno));
            perror("error in delete receive ack 1");
            exit(-1);
        }
        ret = pthread_mutex_unlock(timer->mutex);
        if(ret != 0){
            perror("error in unlock_mutex");
            exit(-1);
        }

        free(timer->id_timer);
        pthread_mutex_destroy(timer->mutex);
        free(timer->mutex);
        free(timer);
        return;
    }
    //printf("EXPIRATION TIMER id = %ld del pkt's seq = %d\n", (long)*(timer->id_timer), timer->pkt->seq);
    struct itimerspec ts;

    ts.it_value.tv_sec = 0 ;
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = 0;
    ts.it_interval.tv_nsec = 0;

    timer_t  id_timer = *(timer->id_timer);

    if (timer_settime(id_timer, 0, &ts, NULL)==-1){
        perror("Error in timer settime: ");
        exit(-1);
    }
    if(*(timer->protocol->flag_timer) == 1) {

        if (timer->pkt->seq == 0 || timer->pkt->seq == *(timer->protocol->win_size)) {
            ts = timer_value_parser(*(timer->protocol->to_interval) * 2);
            timer->to = *(timer->protocol->to_interval) * 2;
        } else {
            ts = timer_value_parser(*(timer->protocol->to_interval));
            timer->to = *(timer->protocol->to_interval);
        }
    } else
        ts = timer_value_parser(*(timer->protocol->to_interval) );


    if (timer_settime(*(timer->id_timer), 0, &ts, NULL)==-1){
        perror("Error in timer settime: ");
        exit(-1);
    }

    /*printf("resubmission pkt with seq %d e con timer_id %ld\n",
           timer->pkt->seq, (long)*(timer->id_timer));*/

    if(get_loss_probability(timer->protocol->loss_p)){
        goto skip_pkt;
    }

    char pack_buff[SIZE_PACK];
    memset((void*)pack_buff, 0, sizeof(pack_buff));
    memmove(pack_buff, timer->pkt, SIZE_PACK);

    if (sendto(*(timer->protocol->sockfd1), (void *)pack_buff, SIZE_PACK, 0,
               (struct sockaddr*)(timer->protocol->addr1),sizeof(*(timer->protocol->addr1))) < 0){
        perror("error in sendto in writeSR");
        exit(-1);
    }

    skip_pkt:

    ret = pthread_mutex_unlock(timer->mutex);
    if(ret != 0){
        perror("error in unlock_mutex");
        exit(-1);
    }

}




void create_timer(struct protocolSR* protocol, struct packet* pkt){


    struct sigevent sev;
    struct itimerspec ts;
    timer_t *id_timer = malloc (sizeof(timer_t));
    memset(id_timer, 0, sizeof(timer_t));
    struct timer* timer = make_timer(protocol, pkt, id_timer);

    (timer->to) = *(protocol->to_interval);

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_value.sival_ptr = timer;
    sev.sigev_notify_function = timer_handler;
    sev.sigev_notify_attributes = NULL;

    ts = timer_value_parser(*(protocol->to_interval));
    try:

    if( timer_create(CLOCK_REALTIME, &sev, id_timer)==-1){
        if (errno == EAGAIN){
            goto try;
        }
        exit(-1);
    }

    add_node(protocol->first_timer, timer);

    if (timer_settime(*id_timer, 0, &ts, NULL)==-1){
        perror("Error in timer settime: ");
        exit(-1);
    }

}


void delete_timer(struct timer** first_timer, int seq){
    int ret = remove_node_and_delete_timer_by_seq(first_timer, seq);
    if(ret == -2){
    }else if(ret == 0){
    }
}



int get_loss_probability(double *loss_p) {
    double n = (double)rand() / (double)RAND_MAX;
    if (n< *(loss_p)){
        return  1;
    }
    return 0;
}

double sampleRTT_support = 0;
void to_calculate(struct protocolSR* protocol, int seq){
    struct  timer* timer = find_node_by_seq(protocol->first_timer, seq);
    if ( timer == NULL){
        return;
    }
    struct itimerspec curr_value;
    timer_t timer_id = *(timer->id_timer);
    if (timer_gettime(timer_id, &curr_value) == -1){
        perror("Error in gettime: ");
        exit(-1);
    }
    if (*(protocol->to_interval) < 0){
        exit(-1);
    }
    long remaining_time = curr_value.it_value.tv_nsec;
    double sampleRTT = timer->to - (double)remaining_time;
    if (protocol->estimated_RTT == NULL){
        sampleRTT_support = sampleRTT;
        (protocol->estimated_RTT) = &sampleRTT_support;
    }
    double estimated_RTT = ((1.0-(*(protocol->alpha))) * (*protocol->estimated_RTT)
                            +  (*(protocol->alpha))* sampleRTT);
    (*protocol->estimated_RTT ) = estimated_RTT;

    double dev_RTT = (1.0-(*(protocol->alpha)*2)) * (*protocol->dev_RTT)
                     +  (*(protocol->alpha)*2)* fabs((sampleRTT-estimated_RTT));

    (*protocol->dev_RTT ) = dev_RTT;
    *(protocol->to_interval) = (long)(estimated_RTT + 4*dev_RTT);


}
