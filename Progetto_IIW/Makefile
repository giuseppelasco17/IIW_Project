client: client.c timer.c SRprotocol.c linked_list.c
	  gcc -o client client.c timer.c SRprotocol.c linked_list.c file_transfer.c -pthread -Wall -Wextra -g -O0 -lrt
server: server.c timer.c SRprotocol.c linked_list.c
	  gcc -o server server.c timer.c SRprotocol.c linked_list.c file_transfer.c -pthread -Wall -Wextra -g -O0 -lrt
clean:
	  rm server && rm client