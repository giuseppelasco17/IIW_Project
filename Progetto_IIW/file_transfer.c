#include "file_transfer.h"


void *receive_ack(void* arg){
    struct protocolSR* protocol = arg;
    for(;;){
        receiveSRack(protocol);
    }
}

void loading_task(int dim_file, int total_byte_read, char* loading_bar_fin){
    double percentage = ((double) total_byte_read / (double) dim_file) * 50;
    char loading_bar[2 * STRING_LENGTH];
    memset(loading_bar, 0, 2 * STRING_LENGTH);

    for (int x = 0; x < percentage; x++) {
        strcat(loading_bar, "|");
    }
    printf("\r%s  %0.2lf%%", loading_bar, percentage * 2);
    fflush(stdout);
    memset(loading_bar_fin, 0, 2 * STRING_LENGTH);
    strcpy(loading_bar_fin, loading_bar);
}

void *write_transfer(void* arg){
    struct protocolSR* protocol = arg;
    char data[DIM_DATA_BLOK];
    char *file_exist;
    int byte_read, total_byte_read = 0;
    char* filename = protocol->filename;
    int fd_file;
    int win_size = *(protocol->win_size);
    fd_file = open(filename,O_RDONLY,0666);
    if (fd_file == -1){
        printf("Send file: Impossible to open %s, the file doesn't exist\n",filename);
        file_exist = "NOT_EXIST";
        if (sendto(*(protocol->sockfd1), (void *)file_exist, SIZE_PACK, 0, (struct sockaddr*)(protocol->addr1),
                   sizeof(*(protocol->addr1))) < 0){
            perror("error in sendto in write_transfer");
            exit(-1);
        }
        pthread_exit(NULL);
    }
    file_exist = "EXIST";
    int dim_file = lseek(fd_file, 0, SEEK_END);
    char dim_file_string[STRING_LENGTH];
    char info[STRING_LENGTH];
    memset(info, 0, sizeof(info));
    memset(dim_file_string, 0, sizeof(dim_file_string));
    strcat(info, file_exist);
    strcat(info, " ");
    snprintf(dim_file_string, 20, "%d", dim_file);
    strcat(info, dim_file_string);
    if (sendto(*(protocol->sockfd1), (void *)info, SIZE_PACK, 0, (struct sockaddr*)(protocol->addr1),
               sizeof(*(protocol->addr1))) < 0) {
        perror("error in sendto in write_transfer");
        exit(-1);
    }
    int seq = 0;
    int ack = 0;

    lseek(fd_file, 0, SEEK_SET);

    protocol->send_base = (protocol->send_base +1) % (*(protocol->win_size));
    char loading_bar_fin[2 * STRING_LENGTH];
    while ((byte_read=read(fd_file, data, sizeof(data))) >0 ){

        struct packet* pack = (struct packet* ) malloc(SIZE_PACK);
        memset((void *)pack, 0, sizeof(*pack));
        make_packet(pack, seq, byte_read, data, ack);

        total_byte_read += byte_read;
        writeSR(dim_file, total_byte_read, pack, protocol);
        seq=(seq+1)% (2*win_size);
        if(protocol->identifier == 0)
            loading_task(dim_file, total_byte_read, loading_bar_fin);
    }

    if(protocol->identifier == 0) {
        printf("\r%s  %0.2lf%%\n", loading_bar_fin, 100.00);
        fflush(stdout);
        printf("\nUpload completed\n");
    }
    if (close(fd_file) == -1){
        perror("Error in close");
    }
    pthread_exit(NULL);
}

void read_transfer(struct protocolSR* protocol, int son_port){
    socklen_t len = sizeof(*(protocol->addr1));
    char info[STRING_LENGTH];

    if ( (recvfrom(*(protocol->sockfd1), info, sizeof(info), 0, (struct sockaddr *)(protocol->addr1),&len ) < 0)) {
        perror("error in recvfrom in read_transfer");
        exit(-1);
    }
    char *file_exist;
    int i = 0, dim_file;
    char delim[] = " ";
    char** info_vector = malloc(2 * STRING_LENGTH);
    char *ptr = strtok(info, delim);

    while (ptr != NULL)
    {
        info_vector[i] = ptr;
        ptr = strtok(NULL, delim);
        i++;
    }
    file_exist = info_vector[0];
    if(strncmp(file_exist, "EXIST", 5) != 0){
        printf("Requested file doesn't exist\n");
        exit(-1);
    }
    dim_file = atoi(info_vector[1]);
    char comm[STRING_LENGTH];
    memset((void*)comm, 0, sizeof(comm));

    char* filename = protocol->filename;
    char dir[] = "../Progetto_IIW/Client_repository/";
    char dir_fin[STRING_LENGTH];
    memset((void*)dir_fin, 0, sizeof(dir_fin));
    snprintf(dir_fin,sizeof(dir_fin),"client_%d/", son_port);
    strcat(dir,dir_fin );
    snprintf(comm, sizeof(comm), "mkdir -p %s", dir);
    system(comm);
    strcat(dir,filename);

    int fd_file;
    fd_file = open(dir,O_CREAT|O_RDWR|O_TRUNC,0666);
    if (fd_file  == -1){
        printf("invio file: impossibile aprire %s \n", filename);
        exit(-1);
    }
    char loading_bar_fin[2 * STRING_LENGTH];


    protocol->fd_file = fd_file;
    protocol->identifier = 0;
    int num_pkts = (int)(dim_file / DIM_DATA_BLOK);
    if(dim_file % DIM_DATA_BLOK != 0){
        num_pkts++;
    }
    while (protocol->end_conn == 0) {
        receiveSR(protocol);
        loading_task(num_pkts , protocol->identifier, loading_bar_fin);
    }
    printf("\r%s  %0.2lf%%\n", loading_bar_fin, 100.00);
    fflush(stdout);
    printf("\nDownload completed\n");

}




