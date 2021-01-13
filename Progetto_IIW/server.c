#include "server.h"

char filename[STRING_LENGTH];
socklen_t len;
int conn_sock_fd, sender_sock_fd, receiver_sock_fd, win_size, fd_file, utility_sock_fd;
int server_son_port = SERV_PORT;
struct sockaddr_in conn_addr, son_addr_sender, son_addr_receiver, utility_sock_addr;
struct packet** window;
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;


struct protocolSR protocol;

struct timer** first_pointer;
double loss_p;

int utility_port = UTILIY_MIN_PORT;

int flag_timer;

long timer_value;


int main(int argc, char *argv[ ]){
    if (argc != 1){
        fprintf(stderr, "usage: %s\n", argv[0]);
        exit(-1);
    }
	signal(SIGCHLD, SIG_IGN);


    struct packet pack;
    memset((void *)&pack, 0, sizeof(pack));


	if ((conn_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error in socket");
        exit(-1);
    }
	memset((void *)&conn_addr, 0, sizeof(conn_addr));
    conn_addr.sin_family = AF_INET;
    conn_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    conn_addr.sin_port = htons(SERV_PORT);

	if (bind(conn_sock_fd, (struct sockaddr *) &conn_addr, sizeof(conn_addr)) < 0) {
		perror("errore in bind");
		exit(-1);
	}

    while (1) {
        connection();

		pid_t son;
		if((son = fork()) < 0){
				perror("errore fork");
				exit(-1);
		}


		/*SON*/
		if(son == 0){
			initialize_transfer_socket();

			len = sizeof(son_addr_receiver);

            char cmd_filename[STRING_LENGTH];
            char client_cmd[STRING_LENGTH];

            memset((void*)client_cmd, 0, sizeof(client_cmd));

            if ( (recvfrom(receiver_sock_fd, client_cmd, sizeof(client_cmd), 0, (struct sockaddr *)&son_addr_receiver, &len)) < 0) {
				perror("error in recvfrom");
				exit(-1);
			}	

			memset((void*)cmd_filename, 0, sizeof(cmd_filename));
			strcpy(cmd_filename, client_cmd);
            strtok(client_cmd, " ");
			
			command_handler(client_cmd, cmd_filename);

			}

		}
	}


void command_handler(char *client_cmd, char* cmd_filename) {
    if(strcmp(client_cmd, "get") == 0){
        strncpy(filename, cmd_filename + 4, strlen(cmd_filename) - 4);
        get();
    } else if(strcmp(client_cmd, "put") == 0){
        strncpy(filename, cmd_filename + 4, strlen(cmd_filename) - 4);
        put();
    } else if(strcmp(client_cmd, "list") == 0){
        list();
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

void list() {
    char *comm = "ls Server_repository > Server_repository/list.txt";
    system(comm);
    char* list = "list.txt";
    memset(filename, 0, STRING_LENGTH);
    strcpy(filename, list);
    get();
}

void put() {

    memset((void *) &protocol, 0, sizeof(protocol));

    window = malloc(2*win_size * sizeof(struct packet *));
    for (int i = 0; i < 2*win_size; i++) {
        window[i] = NULL;
    }

    initializeSR(&protocol, &receiver_sock_fd, &son_addr_receiver, &sender_sock_fd, &son_addr_sender,
                 window, &win_size, NULL, NULL, NULL, NULL, NULL);
    protocol.utility_sock = &utility_sock_fd;
    protocol.utility_addr = &utility_sock_addr;


    char dir[] = "../Progetto_IIW/Server_repository/";



    strcat(dir,filename);


    fd_file = open(dir,O_CREAT|O_RDWR|O_TRUNC,0666);
    if (fd_file  == -1){
        printf("Send file: impossible to open %s \n", filename);
        exit(-1);
    }

    protocol.fd_file = fd_file;
    protocol.identifier = 1;

    char pack_buff[] = "";

    if (sendto(*(protocol.sockfd1), (void *)pack_buff, SIZE_PACK, 0, (struct sockaddr*)(protocol.addr1),
               sizeof(*(protocol.addr1))) < 0){
        perror("error in sendto in put");
        exit(-1);
    }
    len = sizeof(*(protocol.addr1));
    char info[STRING_LENGTH];

    if ( (recvfrom(*(protocol.sockfd1), info, sizeof(info), 0, (struct sockaddr *)(protocol.addr1),&len ) < 0)) {
        perror("error in recvfrom in read_transfer");
        exit(-1);
    }
    char *file_exist;
    int i = 0;
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
    if(strcmp(file_exist, "NOT_EXIST") == 0){
        printf("The requested file does not exist\n");
        if (close(fd_file) == -1){
            perror("Error in close");
        }
        char command[9*STRING_LENGTH];
        snprintf(command, sizeof(command), "rm -r %s", dir);
        system(command);
        exit(-1);
    }
    while (protocol.end_conn == 0) {
        receiveSR(&protocol);
    }
    exit(0);
}

void get() {

    if (bind(utility_sock_fd, (struct sockaddr *) &utility_sock_addr, sizeof(utility_sock_addr)) < 0) {
        perror("error in bind");
        exit(-1);
    }

    memset((void *)&protocol, 0, sizeof(protocol));

    window = malloc(win_size*sizeof(struct packet*));
    for(int i=0; i<win_size;i++){
        window[i] = NULL;
    }


    first_pointer = initialize();

    initializeSR(&protocol, &sender_sock_fd, &son_addr_sender, &receiver_sock_fd, &son_addr_receiver, window,
                 &win_size, &global_lock, first_pointer, &loss_p, &flag_timer, &timer_value);
    char file[] = "Server_repository/";
    strcat(file, filename);
    protocol.filename = file;
    protocol.identifier = 1;
    protocol.utility_sock = &utility_sock_fd;
    protocol.utility_addr = &utility_sock_addr;
    pthread_t receiveAck_tid;
    if((pthread_create(&receiveAck_tid, NULL, receive_ack, (void*)&protocol))!=0){
        perror("error in pthread_create");
        exit(-1);
    }
    pthread_t writeSR_tid;
    if((pthread_create(&writeSR_tid, NULL, write_transfer, (void*)&protocol)!=0)){
        perror("error in pthread_create");
        exit(-1);
    }
    pthread_join(writeSR_tid, NULL);

    if(pthread_cancel(receiveAck_tid) != 0) {
        perror("error in thread kill");
        exit(-1);
    }
    exit(0);
}

void initialize_transfer_socket() {


    if ((utility_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error in open socket");
        exit(-1);
    }
    memset((void *) &utility_sock_addr, 0, sizeof(utility_sock_addr));


    utility_sock_addr.sin_family = AF_INET;
    utility_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    utility_sock_addr.sin_port = htons(utility_port);


    if(close(conn_sock_fd) == -1){
        perror("Error in socket close\n");
        exit(-1);
    }
    if ((sender_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("error in socket");
        exit(-1);
    }
    memset((void *)&son_addr_sender, 0, sizeof(son_addr_sender));
    son_addr_sender.sin_family = AF_INET;
    son_addr_sender.sin_port = htons(server_son_port);

    if ((receiver_sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("errore in socket");
        exit(-1);
    }
    memset((void *)&son_addr_receiver, 0, sizeof(son_addr_receiver));
    son_addr_receiver.sin_family = AF_INET;
    son_addr_receiver.sin_addr.s_addr = htonl(INADDR_ANY);
    son_addr_receiver.sin_port = htons(server_son_port - 1);

    if (bind(receiver_sock_fd, (struct sockaddr *) &son_addr_receiver, sizeof(son_addr_receiver)) < 0) {
        perror("error in bind");
        exit(-1);
    }

}

void connection() {
    char connection_values[STRING_LENGTH], port_string_num[STRING_LENGTH], utility_string_port[STRING_LENGTH];
    memset((void*)connection_values, 0, sizeof(connection_values));
    memset((void*)port_string_num, 0, sizeof(port_string_num));
    memset((void*)utility_string_port, 0, sizeof(utility_string_port));
    len = sizeof(conn_addr);

    if ( (recvfrom(conn_sock_fd, connection_values, sizeof(connection_values), 0, (struct sockaddr *)&conn_addr, &len)) < 0) {
        perror("error in recvfrom");
        exit(-1);
    }
    int i = 0;
    char delim[] = " ";
    char** connection_values_vector = malloc(4 * STRING_LENGTH);
    char *ptr = strtok(connection_values, delim);

    while (ptr != NULL)
    {
        connection_values_vector[i] = ptr;
        ptr = strtok(NULL, delim);
        i++;
    }
    win_size = atoi(connection_values_vector[0]);
    loss_p = atof(connection_values_vector[1]);
    flag_timer = atoi(connection_values_vector[2]);
    timer_value = atol(connection_values_vector[3]);
    server_son_port = ((server_son_port + 2)%(MAX_PORT));
    utility_port = (utility_port+1);
    if (utility_port == MAX_PORT +1){
        utility_port = UTILITY_MAX_PORT;
    }
    if (server_son_port == 0){
        server_son_port = SERV_PORT + 2;
    }

    snprintf(port_string_num, sizeof(port_string_num), "%d", server_son_port);
    strcat(port_string_num, " ");
    snprintf(utility_string_port, sizeof(utility_string_port), "%d", utility_port);
    strcat(port_string_num, utility_string_port);

    port_string_num[sizeof(port_string_num) - 1] = 0;

    if(sendto(conn_sock_fd, port_string_num, strlen(port_string_num), 0, (struct sockaddr*) &conn_addr, sizeof(conn_addr))<0){
        perror("error in sendto");
        exit(-1);
    }
    printf("Client successfully connected\nAssigned ports %d - %d \n", server_son_port - 1, server_son_port);

}
