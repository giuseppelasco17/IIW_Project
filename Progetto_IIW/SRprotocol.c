#include "SRprotocol.h"


double alpha = 0.125;
void initializeSR(struct protocolSR *protocol, int *sockfd1, struct sockaddr_in *addr1, int *sockfd2,
                  struct sockaddr_in *addr2, struct packet **window, int *win_size, pthread_mutex_t *global_lock,
                  struct timer **first_timer, double *loss_p, int* flag_timer, long* timer_value) {

    int ret;
    if(global_lock != NULL){
        ret = pthread_mutex_init(global_lock, NULL);
        if(ret != 0){
            perror("Errore in mutex_init");
            exit(-1);
        }
    }

    sem_t* sem = malloc(sizeof(sem_t));

    ret = sem_init(sem,0, *win_size);
    if(ret != 0) {
        perror("error in sem_init");
        exit(-1);
    }

    protocol->sem = sem;
    protocol->global_lock = global_lock;
    protocol->sockfd1 = sockfd1;
    protocol->addr1 = addr1;
    protocol->sockfd2 = sockfd2;
    protocol->addr2 = addr2;
    protocol->window = window;
    protocol->win_size = win_size;
    protocol->send_base = -1;
    protocol->end_base = 0;
    protocol->rcv_send_base_seq = 0;
    protocol->loss_p = loss_p;
    protocol->flag_timer = flag_timer;

    double dev_RTT = 0.0;
    double *estimated_RTT = NULL;
    protocol->alpha = &alpha;
    (protocol->to_interval) = timer_value;
    (protocol->estimated_RTT) = estimated_RTT;
    (protocol->dev_RTT) = &dev_RTT;


    if(first_timer == NULL){
        protocol->first_timer = NULL;
    }else{
        protocol->first_timer = first_timer;
    }

}

void extract_ack(char* recvData, size_t dimrecvData, struct ack* ack){
    memmove(ack, recvData, dimrecvData);
}

void extract_pack(char* recvData, size_t dimrecvData, struct packet* pack){
    memmove(pack, recvData, dimrecvData);
}
void make_ack(struct ack* ack, int ack_param, int seq){
    ack->ack = ack_param;
    ack->seq = seq;
}

void make_packet(struct packet* pack, int seq, int dimdata, const char* data, int ack){
    pack->ack=ack;
    pack->seq=seq;
    pack->dim_data=dimdata;
    memmove(pack->data, data, dimdata);
}



void writeSR(int dim_file, int byte_read, struct packet* pack, struct protocolSR* protocol){

    //printf("************START WRITE SR************************\n");

    if(byte_read == dim_file){//control last pkt
        pack->ack = -1;
    }

    //lock window
    int ret;
    ret = sem_wait(protocol->sem);
    if(ret != 0){
        perror("errore in sem_wait");
        exit(-1);
    }

    //lock protocol
    ret = pthread_mutex_lock(protocol->global_lock);
    if(ret != 0){
        perror("errore in lock_mutex");
        exit(-1);
    }
    create_timer(protocol, pack);

    if(get_loss_probability(protocol->loss_p)){
        goto skip_pkt;
    }

    char pack_buff[SIZE_PACK];
    memset((void*)pack_buff, 0, sizeof(pack_buff));
    memmove(pack_buff, pack, SIZE_PACK);

    if (sendto(*(protocol->sockfd1), (void *)pack_buff, SIZE_PACK, 0, (struct sockaddr*)(protocol->addr1),
               sizeof(*(protocol->addr1))) < 0){
        perror("error in sendto in writeSR");
        exit(-1);
    }

    skip_pkt:

    if(protocol->window[protocol->end_base] != NULL){
        protocol->end_base = (protocol->end_base +1) % (*(protocol->win_size));
        protocol->window[protocol->end_base] = pack;
    }else{
        protocol->window[protocol->end_base] = pack;
    }
    /*printf("PACKET SENDS with seq = %d, pid = %d, send_base = %d, end_base = %d\n",pack->seq, getpid(),
           protocol->send_base, protocol->end_base );*/
    //printf("************END WRITE SR************************\n");

    ret = pthread_mutex_unlock(protocol->global_lock);
    if(ret != 0){
        perror("errore in unlock_mutex");
        exit(-1);
    }


    if (pack->ack == -1){
        char buff_ack[SIZE_ACK];
        memset((void*)buff_ack, 0, sizeof(buff_ack));

        socklen_t len = sizeof(*(protocol->utility_addr));
        if ( (recvfrom(*(protocol->utility_sock), buff_ack, sizeof(buff_ack), 0, (struct sockaddr *)(protocol->utility_addr),&len ) < 0)) {
            perror("error in recvfrom");
            exit(-1);
        }
    }
}


void receiveSRack(struct protocolSR* protocol ){
    int ret;

    struct ack ack;
    char buff_ack[SIZE_ACK];
    memset((void*)buff_ack, 0, sizeof(buff_ack));

    socklen_t len = sizeof(*(protocol->addr2));
    if ( (recvfrom(*(protocol->sockfd2), buff_ack, sizeof(buff_ack), 0, (struct sockaddr *)(protocol->addr2),&len ) < 0)) {
        perror("error in recvfrom");
        exit(-1);
    }

    //lock protocol
    ret = pthread_mutex_lock(protocol->global_lock);
    if(ret != 0){
        perror("error in lock_mutex");
        exit(-1);
    }
    //printf("************START RECEIVE SR ACK************************\n");
    extract_ack(buff_ack, sizeof(buff_ack), &ack);

    if ((protocol->window)[protocol->send_base] == NULL){
        goto unlock_mutex;
    }

    int send_base_seq = (protocol->window)[protocol->send_base]->seq;
    int count = 0;

    for(int i = 0; i<*(protocol->win_size); i++){
        int x;
        x = (send_base_seq + i)   %    (   (*(protocol->win_size)*(2))        ) ;
        if ( x == ack.seq){
            if(*(protocol->flag_timer) == 1)
                to_calculate(protocol, ack.seq);
            if (i == 0){

                //send_base
                delete_timer(protocol->first_timer, ack.seq);
                (protocol->window)[protocol->send_base]->ack = 1;

                int k = protocol->send_base;
                while((protocol->window[k] != NULL)){
                    if((protocol->window)[k]->ack == 1) {

                        //free pkt
                        free(protocol->window[k]);
                        protocol->window[k] = NULL;
                        ret = sem_post(protocol->sem);
                        if(ret != 0){
                            perror("error in lock_mutex");
                            exit(-1);
                        }
                        k = (k + 1) % (*(protocol->win_size));
                        count++;
                    }else{
                        break;
                    }
                }
                int index = k;
                if (  ( (protocol->end_base + 1) % ( *(protocol->win_size)   )  ) == index   ){
                    protocol->send_base =( (index+1) % ( *(protocol->win_size) )  );
                    protocol->end_base =( (index+1) % ( *(protocol->win_size)   )  );
                }else{
                    protocol->send_base = (index) % ( *(protocol->win_size)   );
                }

            }else{

                //not send base
                delete_timer(protocol->first_timer, ack.seq);

                int difference = 1;
                for (; difference < *(protocol->win_size); difference++){

                    int y = (send_base_seq +  difference) % (*(protocol->win_size)*2);
                    if (y == ack.seq){
                        break;
                    }
                }

                int slot = (difference + protocol->send_base) % ( *(protocol->win_size)   );

                if ( (protocol->window)[slot]->seq == ack.seq){
                    (protocol->window)[slot]->ack = 1;
                }
            }
            goto unlock_mutex;
        }

    }
    unlock_mutex:

    //printf("************END RECEIVE SR ACK************************\n");
    ret = pthread_mutex_unlock(protocol->global_lock);
    if(ret != 0){
        perror("errore in unlock_mutex");
        exit(-1);
    }
}


void receiveSR(struct protocolSR* protocol){


    char buff_pack[SIZE_PACK];
    memset((void*)buff_pack, 0, sizeof(buff_pack));
    struct packet* pack = (struct packet*) malloc(SIZE_PACK);
    socklen_t len = sizeof(*(protocol->addr1));

    int rets = recvfrom(*(protocol->sockfd1), (void*)buff_pack, sizeof(buff_pack), 0, (struct sockaddr *)(protocol->addr1), &len);
    if ( (rets < 0)) {
        perror("error in recvfrom");
        exit(-1);
    }
    extract_pack(buff_pack, sizeof(buff_pack), pack);
    protocol->pack = pack;
    receive_routine(protocol);

}




void receive_routine(struct protocolSR* protocol){

    struct packet* pack = protocol->pack;

    int seq_pack = pack->seq;

    int win_size = *protocol->win_size;

    int sb_seq = protocol->rcv_send_base_seq;


    if(!(pack->ack == -1 && sb_seq == pack->seq)){

        if(pack->ack == -1){
            return;
        }

        for(int i = 0; i<*(protocol->win_size); i++) {
            int x;
            x = (sb_seq + i) % ((*(protocol->win_size) * (2)));
            if (x == pack->seq) {//packet acceptance
                (protocol->window)[(pack->seq)] = pack;
                if (i == 0) {//send_base
                    int count = 0;
                    while (((protocol->window[(sb_seq + count) % (2 * win_size)]) != NULL) && count < win_size) {

                        write_file(protocol, (protocol->window)[(sb_seq + count) % (2 * win_size)]);
                        free((protocol->window)[(sb_seq + count) % (2 * win_size)]);
                        (protocol->window)[(sb_seq + count) % (2 * win_size)] = NULL;

                        count++;
                        protocol->identifier++;
                    }
                    protocol->rcv_send_base_seq = (sb_seq + count) % (2 * win_size);
                }
                break;
            }
        }
    }else{
        //Last pkt
        write_file(protocol, protocol->pack);
        protocol->identifier++;

        struct ack ack;

        make_ack(&ack, 1, seq_pack);

        char buff_ack[SIZE_ACK];
        memset((void*)buff_ack, 0, sizeof(buff_ack));
        memmove(buff_ack, &ack, SIZE_ACK);

        if (sendto(*(protocol->sockfd2), (void *)buff_ack, sizeof(buff_ack), 0, (struct sockaddr*)(protocol->addr2),
                   sizeof(*(protocol->addr2))) < 0){
            perror("error in sendto in receive_routine");
            exit(-1);
        }


        //send ack -1 to close connection
        struct ack last_ack;
        make_ack(&last_ack, -1, seq_pack);


        memset((void*)buff_ack, 0, sizeof(buff_ack));
        memmove(buff_ack, &last_ack, SIZE_ACK);

        if (sendto(*(protocol->utility_sock), (void *)buff_ack, sizeof(buff_ack), 0, (struct sockaddr*)(protocol->utility_addr),
                   sizeof(*(protocol->utility_addr))) < 0){
            perror("errore in sendto in receive_routine");
            exit(-1);
        }
        goto last_pack;
    }



    //send ack
    struct ack ack;
    make_ack(&ack, 1, seq_pack);

    char buff_ack[SIZE_ACK];
    memset((void*)buff_ack, 0, sizeof(buff_ack));
    memmove(buff_ack, &ack, SIZE_ACK);

    //printf("SENDS ACK %d\n", seq_pack);
    if (sendto(*(protocol->sockfd2), (void *)buff_ack, sizeof(buff_ack), 0, (struct sockaddr*)(protocol->addr2),
               sizeof(*(protocol->addr2))) < 0){
        perror("errore in sendto in receive_routine");
        exit(-1);
    }

    return;

    last_pack:

    free(pack);
    protocol->end_conn = -1;
}


void write_file(struct protocolSR* protocol, struct packet* pack){

    if (write(protocol->fd_file, pack->data ,pack->dim_data) != pack->dim_data){
        perror("Error in write in write_file");
        exit(-1);
    }


}

