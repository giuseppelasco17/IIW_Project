
#include "client.h"

int conn_sock_fd, receiver_sock_fd, sender_sock_fd, utility_sock_fd;
char serv_son_port[sizeof(struct packet)];
struct sockaddr_in servaddr, son_servaddr_receiver, son_servaddr_sender, utility_sock_addr;
int fd_file, son_port;
char filename[STRING_LENGTH];
char* serv_ip_address;
char* win_size_string;
struct packet** window;
int win_size;
struct protocolSR protocol;
char cmd_filename[STRING_LENGTH];
double loss_p;
char usr_command[STRING_LENGTH];
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

int utility_port;


struct timer **first_pointer;

char *flag_timer_string;

int flag_timer;

char timer_value_string[STRING_LENGTH];

char connection_values[STRING_LENGTH];

char *loss_p_string;

long timer_value;


void user_interface(void){
   

  while(1){
    memset(connection_values, 0, STRING_LENGTH);
    strcat(connection_values, win_size_string);
    strcat(connection_values, " ");
    strcat(connection_values, loss_p_string);
    strcat(connection_values, " ");
    strcat(connection_values, flag_timer_string);
    strcat(connection_values, " ");
    strcat(connection_values, timer_value_string);
    connection();

    //Create son to manage the connection
    pid_t son;
    if((son = fork()) < 0){
      perror("errore fork");
      exit(-1);
    }

    /*SON*/
    if(son == 0) {

        initialize_socket_and_send_command();
        command_handler_client();

    }
    if(waitpid(son, NULL, 0) == -1){
      perror("errore in wait pid");
      exit(-1);
    }
  }
}

void command_handler_client(){
    if(strcmp(usr_command, "get") == 0){
        get_client();
    } else if(strcmp(usr_command, "put") == 0){
        put_client();
    } else if(strcmp(usr_command, "list") == 0){
        list_client();
    } else{
        printf("Non-existent command\n");
        if(close(sender_sock_fd) == -1){
            perror("Error in socket close\n");
            exit(-1);
        }
        if(close(receiver_sock_fd) == -1){
            perror("Error in socket close\n");
            exit(-1);
        }
        exit(-1);
    }
}

void list_client(){
    char* list = "list.txt";
    memset((void *) &protocol, 0, sizeof(protocol));

    window = malloc(2*win_size * sizeof(struct packet *));
    for (int i = 0; i < 2*win_size; i++) {
        window[i] = NULL;
    }

    initializeSR(&protocol, &receiver_sock_fd, &son_servaddr_receiver, &sender_sock_fd, &son_servaddr_sender,
                 window, &win_size, NULL, NULL, NULL, NULL, NULL);
    protocol.filename = list;
    protocol.identifier = 0;
    protocol.utility_sock = &utility_sock_fd;
    protocol.utility_addr = &utility_sock_addr;
    read_transfer(&protocol, son_port);
    char word[STRING_LENGTH], comm[8*STRING_LENGTH], command[9*STRING_LENGTH];
    FILE *file;
    char dir[] = "../Progetto_IIW/Client_repository/";
    char dir_fin[5*STRING_LENGTH];
    memset((void*)dir_fin, 0, sizeof(dir_fin));
    snprintf(dir_fin,sizeof(dir_fin),"client_%d/", son_port);
    strcat(dir,dir_fin );
    strcpy(comm, dir);
    strcat(dir,list);
    file = fopen(dir, "r");
    printf("\n");
    while (fscanf(file,"%s",word)>0)
    {
        printf("%s\n", word);
        fflush(stdout);
    }
    printf("\n");
    fclose(file);
    snprintf(command, sizeof(command), "rm -r %s", comm);
    system(command);
    exit(0);
}

void put_client(){
    if (bind(utility_sock_fd, (struct sockaddr *) &utility_sock_addr, sizeof(utility_sock_addr)) < 0) {
        perror("errore in bind");
        exit(-1);
    }

    strncpy(filename, cmd_filename + 4, strlen(cmd_filename) - 4);
    char file_dir[] = "Client_repository/";
    strcat(file_dir, filename);
    memset((void *)&protocol, 0, sizeof(protocol));

    window = malloc(win_size*sizeof(struct packet*));
    for(int i=0; i<win_size;i++){
        window[i] = NULL;
    }


    first_pointer = initialize();

    initializeSR(&protocol, &sender_sock_fd, &son_servaddr_sender, &receiver_sock_fd, &son_servaddr_receiver, window,
                 &win_size, &global_lock, first_pointer, &loss_p, &flag_timer, &timer_value);
    protocol.filename = file_dir;
    protocol.identifier = 0;
    protocol.utility_sock = &utility_sock_fd;
    protocol.utility_addr = &utility_sock_addr;
    char buff_ready[SIZE_ACK];
    memset((void*)buff_ready, 0, sizeof(buff_ready));

    socklen_t len = sizeof(*(protocol.addr1));
    if ( (recvfrom(*(protocol.sockfd1), buff_ready, sizeof(buff_ready), 0,
            (struct sockaddr *)(protocol.addr1),&len ) < 0)) {
        perror("errore in recvfrom in put");
        exit(-1);
    }

    pthread_t receiveAck_tid;
    if((pthread_create(&receiveAck_tid, NULL, receive_ack, (void*)&protocol))!=0){
        perror("errore in pthread_create");
        exit(-1);
    }
    pthread_t writeSR_tid;

    if((pthread_create(&writeSR_tid, NULL, write_transfer, (void*)&protocol)!=0)){
        perror("errore in pthread_create");
        exit(-1);
    }
    pthread_join(writeSR_tid, NULL);

    if(pthread_cancel(receiveAck_tid) != 0){
        perror("error in thread kill");
        exit(-1);
    }
    exit(0);
}


void get_client(){
    strncpy(filename, cmd_filename + 4, strlen(cmd_filename) - 4);

    memset((void *) &protocol, 0, sizeof(protocol));

    window = malloc(2*win_size * sizeof(struct packet *));
    for (int i = 0; i < 2*win_size; i++) {
        window[i] = NULL;
    }

    initializeSR(&protocol, &receiver_sock_fd, &son_servaddr_receiver, &sender_sock_fd, &son_servaddr_sender,
                 window, &win_size, NULL, NULL, NULL, NULL, NULL);
    protocol.filename = filename;
    protocol.identifier = 0;
    protocol.utility_sock = &utility_sock_fd;
    protocol.utility_addr = &utility_sock_addr;
    read_transfer(&protocol, son_port);
    exit(0);
}

void initialize_socket_and_send_command(){


    if ((utility_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error in socket");
        exit(-1);
    }
    memset((void *) &utility_sock_addr, 0, sizeof(utility_sock_addr));


    utility_sock_addr.sin_family = AF_INET;

    utility_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    utility_sock_addr.sin_port = htons(utility_port);
    if (inet_pton(AF_INET, serv_ip_address, &utility_sock_addr.sin_addr) <= 0) {
        perror("error in inet_pton");
        exit(-1);
    }

    if ((receiver_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error in socket");
        exit(-1);
    }

    memset((void *) &son_servaddr_receiver, 0, sizeof(son_servaddr_receiver));


    son_servaddr_receiver.sin_family = AF_INET;
    son_servaddr_receiver.sin_addr.s_addr = htonl(INADDR_ANY);
    son_servaddr_receiver.sin_port = htons(son_port);
    if (inet_pton(AF_INET, serv_ip_address, &son_servaddr_receiver.sin_addr) <= 0) {
        perror("error in inet_pton");
        exit(-1);
    }



    if (bind(receiver_sock_fd, (struct sockaddr *) &son_servaddr_receiver, sizeof(son_servaddr_receiver)) < 0) {
        perror("error in bind");
        exit(-1);
    }
    if ((sender_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error in socket");
        exit(-1);
    }

    memset((void *) &son_servaddr_sender, 0, sizeof(son_servaddr_sender));


    son_servaddr_sender.sin_family = AF_INET;
    son_servaddr_sender.sin_port = htons(son_port - 1);
    if (inet_pton(AF_INET, serv_ip_address, &son_servaddr_sender.sin_addr) <= 0) {
        perror("error in inet_pton");
        exit(-1);
    }

    if (sendto(sender_sock_fd, usr_command, strlen(usr_command), 0, (struct sockaddr *) &son_servaddr_sender,
               sizeof(son_servaddr_sender)) < 0) {
        perror("error in sendto");
        exit(-1);
    }
    memset((void *) cmd_filename, 0, sizeof(cmd_filename));
    strcpy(cmd_filename, usr_command);
    strtok(usr_command, " ");
}

void connection(){
    if ((conn_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error in socket");
        exit(-1);
    }
    memset((void *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (inet_pton(AF_INET, serv_ip_address, &servaddr.sin_addr) <= 0) {
        perror("errore in inet_pton");
        exit(-1);
    }
    if(sendto(conn_sock_fd, connection_values, strlen(connection_values), 0, (struct sockaddr*) &servaddr,
            sizeof(servaddr))<0){
        perror("error in sendto");
        exit(-1);
    }
    int n = recvfrom(conn_sock_fd, serv_son_port, MAXLINE, 0 , NULL, NULL);
    if ( (n < 0)) {
        perror("error in recvfrom");
        exit(-1);
    }

    char delim[] = " ";
    int i = 0;
    char** info_vector = malloc(2 * STRING_LENGTH);
    char *ptr = strtok(serv_son_port, delim);

    while (ptr != NULL)
    {
        info_vector[i] = ptr;
        ptr = strtok(NULL, delim);
        i++;
    }
    char utility[STRING_LENGTH] ;

    strcpy(serv_son_port, info_vector[0]);
    strcpy(utility, info_vector[1]);

    printf("\n*** Assigned Folder: client_%s ***\n", serv_son_port);
    son_port = atoi(serv_son_port);
    utility_port = atoi(utility);

    printf("Insert command (list, get <file_name>, put <file_name>, quit):\n");
    memset((void *) usr_command, 0, sizeof(usr_command));
    fflush(stdin);
    fgets(usr_command, STRING_LENGTH, stdin);
    strtok(usr_command, "\n");
    if (strncmp(usr_command, "quit", 4) == 0) {
        exit(0);
    }
}


void parser(){

    if (win_size <= 0){
        printf("Win size must be greater than 0\n");
        exit(-1);
    }

    if (loss_p < 0 || loss_p > 1){
        printf("Loss probability must be between 0 and 1\n");
        exit(-1);
    }

    if (flag_timer != 0 && flag_timer != 1){
        printf("Flag timer must be 0 or 1.\n0 if you want non-adaptive timer, 1 otherwise\n");
        exit(-1);
    }


}


/*takes the parameters from the command line and delegates the control of the
 * correctness of the latter to the parser function and calls the user inerface function.*/
int main(int argc, char *argv[]){
	if(argc != 6){
		fprintf(stderr,"Usage: %s <ip_server> <window_size> <loss_probability> <flag_timer> <timer_value_in_millisec>\n"
		        , argv[0]);
		exit(-1);
	}
	serv_ip_address = argv[1];
    win_size = atoi(argv[2]);
    win_size_string = argv[2];
    loss_p = atof(argv[3]);
    loss_p_string = argv[3];
    flag_timer_string = argv[4];
    flag_timer = atoi(flag_timer_string);
    strcat(timer_value_string, argv[5]);
    strcat(timer_value_string, "000000");//conversion from millisecond to nanosecond
    timer_value = atol(timer_value_string);

    parser();
    user_interface();
	return 0;
}


