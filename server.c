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


	

int search_value(int key, char delete, dictionary* ret){

	dictionary *aux_actual, *aux_last;
	aux_actual = aux_last = first_entry;

	while(aux_actual!=last_entry) {//until last element
		
		if(aux_actual->key == key)//found it!
			if (delete){
				aux_last->next=aux_actual->next;
				free(aux_actual->value);
				free(aux_actual);
				return 0;
			}
			ret = aux_actual;
			return 0;
		
		aux_last=aux_actual;
		aux_actual = aux_actual->next;
	}
	
	return -1;	// not in dictionary

}

void* handle_requests(void* arg) {

	int socket_fd = (int) arg;	//saves socket file descriptor from client
	int nbytes;
	int aux;

	kv_client2server message;

	printf("Hello thread world!");
	

	/* read message */
	nbytes = recv(socket_fd, &message, sizeof(message), 0);

	printf("Received message");

	if(message.op == 'r') {
		// search the dictionary
		dictionary* kv_entry;
		int search_error = search_value(message.key, 0, kv_entry);
		
		// entry not in dictionary
		if(search_error == -1) { 
			message.error_code = -2;
			nbytes = send(socket_fd , &message, sizeof(message), 0);
			return ((int) -1);
		}
		
		// send message header 
		nbytes = send(socket_fd , (dictionary*) kv_entry->value_length, sizeof(int), 0);
	
		//send message
		message.error_code = 0;
		nbytes = send(socket_fd, kv_entry->value, kv_entry->value_length, 0);
		if(nbytes != kv_entry->value_length) {
			perror("send failed");
			return ((int) -1);
		}
		
	}
	
	if(message.op == 'w') {
		
		dictionary* kv_entry = (dictionary*) malloc(sizeof(dictionary));
		kv_entry->value = malloc(message.value_length);//allocate the necessary space for the message value

		nbytes = recv(socket_fd, kv_entry->value , message.value_length , 0);//only read up to param value_length
 
		if(nbytes != message.value_length) {
			perror("receive values failed");
			return ((int) -1);
		}

		// check if given key already exists
		dictionary* search_entry;
		int search_error = search_value(kv_entry->key, 0, search_entry);
		if(search_error == 0) {//entry already exists
			if(!message.overwrite)
				return ((int) -2);	// value already exists and is not overwritten
			else
				//do overwrite
				search_entry->value_length = message.value_length;
				free(search_entry->value);
				search_entry->value = malloc(message.value_length);

				if(memcpy(search_entry->value, kv_entry->value, message.value_length)== NULL)
					perror("memcpy");

				free(kv_entry->value);
				free(kv_entry);
				return 0;
		}
		
		//store values
		kv_entry->key=message.key;
		kv_entry->value_length=message.value_length;

		kv_entry->next =NULL;
		if(first_entry == NULL) {
			first_entry = kv_entry;
			last_entry = kv_entry;
		}else {
			last_entry->next = kv_entry;
			last_entry = kv_entry;			
		}

		return 0;
		
	}

	if(message.op == 'd') {
		// search the dictionary
		int search_error = search_value(message.key, 1, NULL);
		
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
	err = inet_aton("127.0.0.1", &(local_addr.sin_addr));
	if(err == -1) {
		perror("inet_aton");
		exit(-1);
	}
	//local_addr.sin_addr.s_addr = INADDR_ANY;
	

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

		new_socket = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);
		if(new_socket == 0) {
			perror("socket accept");
			exit(-1);
		}
		err = pthread_create(&tid, NULL, handle_requests, &new_socket);
		if(err!=0) {
			perror("pthread_create");
			exit(-1);
		}
		
		printf("Request accepted\n");
        
	}

	
	exit(0);
    
}
