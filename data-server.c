#include "psiskv.h"
#include "database.h"

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
	struct dictionary* prev;
}dictionary;

dictionary* first_entry = NULL;
dictionary* last_entry = NULL;


int read_db(int, kv_client2server);
int write_db(int, kv_client2server);
int delete_db(int, kv_client2server);
int close_db(int);



//TODO: transformar o log num backup periodicamente, por exemplo, no principio e no fim
void log() {

	fopen(LOG_FILE, "a");
	
	
	
	
	
	
	fclose(LOG_FILE);

}



void * handle_requests(void* arg) {

	int socket_fd = *((int*) arg);
	int nbytes;
	int aux;
	int i=0;

	kv_client2server message;

	//DEBUG
	printf("Hello thread world! ckt=%d\n", socket_fd); fflush(stdout);
	
	while(1) {
		
		//DEBUG
		printf("start of loop, i=%d\n", i++); fflush(stdout);
		
		/* read message */
		nbytes = recv(socket_fd, &message, sizeof(message), 0);

		//DEBUG
		printf("nbytes: %d\n", nbytes); fflush(stdout);
		printf("message:\n"); fflush(stdout);
		printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
				message.op, message.key, message.value_length, message.overwrite, message.error_code); 
		fflush(stdout);

		printf("Received message\n"); fflush(stdout);
		
		

		if(message.op == 'r') {
			read_db(socket_fd, message);
		}
	
		if(message.op == 'w') {
			write_db(socket_fd, message);
		}

		if(message.op == 'd') {
			delete_db(socket_fd, message);
		}

		if(message.op == 'c') {
			close_db(socket_fd);
			// break out of loop if client wants t oclose connection
			break;
		}
		

	}//end while
	
	// terminate this thread
	pthread_exit();
	
}//end handle_requests






void read_db(int socket_fd, kv_client2server message) {

	//DEBUG
	printf("READING\n"); fflush(stdout);
	
	// search the dictionary
	dictionary kv_entry = find_entry(message.key);

	//DEBUG
	printf("after search: err=%d\n", search_error); fflush(stdout);

	// entry not in dictionary
	if(search_error == NULL) { 
		printf("search error / not in dict\n"); fflush(stdout);
		message.error_code = -2;
		nbytes = send(socket_fd , &message, sizeof(message), 0);
		pthread_exit(&message.error_code);
		//TODO:fazer o thread_join para libertar os recursos da thread terminada
	}

	//DEBUG
	printf("message:\n"); fflush(stdout);
	printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
			message.op, message.key, message.value_length, message.overwrite, message.error_code); 
	fflush(stdout);

	// send message header to client with size of msg
	message.value_length = kv_entry->value_length;
	nbytes = send(socket_fd , &message, sizeof(int), 0);

	//send message
	message.error_code = 0;
	nbytes = send(socket_fd, kv_entry->value, kv_entry->value_length, 0);
	if(nbytes != kv_entry->value_length) {
		perror("send failed");
		pthread_exit(&kv_entry->value_length);
	}

}




void write_db(kv_client2server message) {

	//DEBUG
	printf("WRITING\n"); fflush(stdout);

	dictionary* kv_entry = (dictionary*) malloc(sizeof(dictionary));		
	kv_entry->value = malloc(message.value_length);//allocate the necessary space for the message value

	nbytes = recv(socket_fd, kv_entry->value , message.value_length , 0);//only read up to param value_length
	printf("after recv\n");fflush(stdout);
	if(nbytes != message.value_length) {
		perror("receive values failed");
		return -1;
	}

	// check if given key already exists
	dictionary* search_entry;
	int search_error = add_entry(kv_entry->key, kv_entry->value, kv_entry->value_length, message.overwrite);
	if(search_error==-1) {
		//TODO:...
	}

	//DEBUG
	printf("end of check if given key already exists\n");fflush(stdout);

}




void delete_db(int soket_fd, kv_client2server message) {

	// search the dictionary
	int search_error = search_value(message.key, 'd', NULL, 0);

	// entry not in dictionary
	if(search_error == -1) { 
		message.error_code = -2;
		nbytes = send(socket_fd , &message, sizeof(message), 0);
		return (int) -1;
	}

	if(search_error == 0)
		message.error_code = 0;		

	// send message ack
	nbytes = send(socket_fd , &message, sizeof(message), 0);

}





void close_db(int socket_fd) {
	close(socket_fd);
}



int initialisation() {

	//TODO: ler backup e preencher o dictionary
	//TODO: definir estrutura para guardar no backup, ex:
	//			uma entry:	key
	//						value_length
	//						value
	
	dictionary kv_entry;
		
	FILE *backup_fptr = fopen(BACKUP_FILE, "rb");
	if(backup_fptr == NULL)
		return -1;
	
	while(feof(backup_fptr)) {
		
		// read key
		fread(&(kv_entry->key), sizeof(int), 1, backup_fptr);
		
		// read value_length
		fread(&(kv_entry->value_length), sizeof(int), 1, backup_fptr);
		
		// allocate memory for value
		kv_entry->value = malloc(kv_entry->value_length);
		
		// read value
		fread(kv_entry->value, 1, kv_entry->value_length, backup_fptr);
		
		// add entry to dictionary
		add_entry(kv_entry);
	}
	
	return 0;

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
	
	
	/* initialisation */
	initialisation();

	
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
		new_socket = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);
		if(new_socket == 0) {
			perror("socket accept");
			exit(-1);
		}

		printf("after accept sck=%d\n", new_socket);
		err = pthread_create(&tid, NULL, handle_requests, (void *) (&new_socket));
		if(err!=0) {
			perror("pthread_create");
			exit(-1);
		}
		
		printf("Request accepted\n");
        
	}

	
	exit(0);
    
}
