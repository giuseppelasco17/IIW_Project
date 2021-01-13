#ifndef STRUCTURE_H
#define STRUCTURE_H

/*This header contains all the macros and structures used by the program*/


#include <sys/types.h>
#include <semaphore.h>

#define DIM_DATA_BLOK 1300
#define SERV_PORT 5193
#define MAXLINE 1300
#define MAX_PORT 43688
#define UTILIY_MIN_PORT 43689
#define UTILITY_MAX_PORT 65534
#define STRING_LENGTH 100




struct packet{
    int seq;//packet's sequence number
    int ack;//it shows if the packet has been received or not
    int dim_data;//packet's size
    char data[DIM_DATA_BLOK];//packet's data
};


struct ack{
    int seq;//ack's sequence number
    int ack;//signlas the correct reception of the last package or not
};

struct timer{
    timer_t *id_timer;
    struct packet* pkt;//packet associated with the timer
    struct timer* next_timer;//next timer in the linked list
    struct protocolSR* protocol;
    pthread_mutex_t* mutex;//the arrival of the ack and the expiration of the timer can occur in competition
    int arrived;//flag
    long to;//time out interval
};

#define SIZE_PACK sizeof(struct packet)
#define SIZE_ACK sizeof(struct ack)
#define TIMER_NANO 999999999
#define TIMER_MAX 499999999
#define TIMER_SEC 1

struct protocolSR{
    /*sockfd1 and sockfd2 represent the descriptor files of the sockets
     * used for sender and receiver communication.
     * */
    int* sockfd1;
    struct sockaddr_in* addr1;

    int* sockfd2;
    struct sockaddr_in* addr2;

    int* utility_sock;//socket used for secure communication closure
    struct sockaddr_in* utility_addr;

    struct packet** window;//window where the packages to be sent/received are stored
    int* win_size;//window's size

    int fd_file;//file's file descriptor
    char* filename;//file's name

    pthread_mutex_t* global_lock;// mutex used to manage the competition on the variables common to the threads
    // that deal with the sending of data and the related ack

    int send_base;//send base sender side
    int end_base;//end base sender side

    int rcv_send_base_seq; //send base receive side

    struct packet* pack;//support pointer

    int end_conn;//end connection flag
    struct timer** first_timer;//head timers' linked list


    double *loss_p;//loss probability
    sem_t *sem;//manage number of packets in the window

    long* to_interval;//time out interval
    double * alpha,*estimated_RTT, *dev_RTT;//adaptive timers' parameters
    int *flag_timer;//adaptive timer or not

    int identifier;//0-client; 1-server
};

#endif
