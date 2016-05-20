#include "psiskv.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

typedef struct dictionary{
	int key;
	int value_length;
	void* value;
	struct dictionary* next;
}dictionary;

dictionary* first_entry = NULL;
dictionary* last_entry = NULL;



//TODO: tratar do CTRL+C
void signal_handler() {

}


void* handle_requests(void* arg) {

	int socket_fd = (int*) arg;	//saves socket file descriptor from client
	int nbytes;
	int aux;
	int i=0;

	kv_client2server message;

	printf("Hello thread world! ckt=%d\n", socket_fd); fflush(stdout);
	
	while(1) {
		
		printf("start of loop, i=%d\n", i++); fflush(stdout);
		/* read message */
		nbytes = recv(socket_fd, &message, sizeof(message), 0);
	
		printf("nbytes: %d\n", nbytes); fflush(stdout);
		printf("message:\n"); fflush(stdout);
		printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
				message.op, message.key, message.value_length, message.overwrite, message.error_code); 
		fflush(stdout);

		printf("Received message\n"); fflush(stdout);


		if(message.op == 'r') {

		
		}
	
		if(message.op == 'w') {


		
		}

		if(message.op == 'd') {

		}

		if(message.op == 'c') {
			
		}


	}//end while
}




int main(){

    kv_client2server m;
	int nbytes;
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	int err;
	int socket_fd;
	int new_socket;
	pthread_t tid;
	int backlog;

	
	/* create socket  */ 
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(PORT);
	//err = inet_aton("127.0.0.1", &(local_addr.sin_addr));
	if(err == -1) {
		perror("inet_aton");
		exit(-1);
	}
	local_addr.sin_addr.s_addr = INADDR_ANY;
	

	/* bind socket */
	err = bind(socket_fd, (struct sockaddr*) &local_addr, sizeof(local_addr));
	if(err == -1) {
		perror("bind");
		exit(-1);
	}

	/* listen */
	err = listen(socket_fd, backlog);
	if(err == -1) {
		perror("listen");
		exit(-1);
	}

	while(1) {
	
		int local_addr_size = sizeof(local_addr);
		int client_addr_size = sizeof(client_addr);

		printf("before accept\n");
		getchar();
		new_socket = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);
		if(new_socket == 0) {
			perror("socket accept");
			exit(-1);
		}

		printf("after accept sck=%d\n", new_socket);
		getchar();
		err = pthread_create(&tid, NULL, handle_requests, (void*) new_socket);
		if(err!=0) {
			perror("pthread_create");
			exit(-1);
		}
		
		printf("Request accepted\n");
        
	}

	
	exit(0);
    
}
