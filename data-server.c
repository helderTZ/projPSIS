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
	struct dictionary* prev;
}dictionary;

dictionary* first_entry = NULL;
dictionary* last_entry = NULL;


	

int search_value(int key, char op, dictionary** ret, int overwrite){

	dictionary *aux_actual, *aux_last;
	aux_actual = aux_last = first_entry;
	
	printf("IN SEARCH_VALUE before loop\n"); fflush(stdout);
	
	if(first_entry==NULL) { // no elements in dict
		if(op=='r' || op=='d')
			return -1;
		if(op=='w') {
			first_entry = *ret;
			first_entry->next == NULL;
			first_entry->prev == NULL;
			return 0;
		}
		return -1;
	}
	
	if(first_entry == last_entry) {	// 1 element in dict
		if(op=='r' && first_entry->key == key) {
			*ret = first_entry;
			return 0;
		}
		if(op=='w' && first_entry->key == key) {
			if(overwrite==1) {
				free(first_entry->value);
				free(first_entry);
				first_entry->value = (*ret)->value;
			} else {
				first_entry->next = *ret;
				(*ret)->next = NULL;
				(*ret)->prev = first_entry;
				return 0;
			}
		}
		if(op=='d' && first_entry->key == key) {
			free(first_entry->value);
			free(first_entry);
			first_entry = NULL;
			return 0;
		}
		return -1;
	}
						
	
	while(aux_actual!=last_entry) {//until last element
	
		printf("IN SEARCH_VALUE after loop\n"); fflush(stdout);
		
		if(aux_actual->key == key) { //found it!
			if(op=='r') {
				*ret = aux_actual;
				return 0;
			}
			if(op=='w') {
				if(overwrite==1) {
					free(aux_actual->value);
					aux_actual->value = malloc((*ret)->value_length);
					memcpy(aux_actual->value, (*ret)->value, (*ret)->value_length);
					return 0;
				}
				else {
					// meter na ultima posiçao da lista
					last_entry->next = (dictionary*)malloc(sizeof(dictionary));
					(last_entry->next)->value = malloc((*ret)->value_length);
					memcpy((last_entry->next)->value, (*ret)->value, (*ret)->value_length);
					(last_entry->next)->prev = last_entry;
					last_entry = last_entry->next;
					return 0;
				}
			}
			if (op == 'd'){
				(aux_actual->prev)->next = aux_actual->next;
				(aux_actual->next)->prev = aux_actual->prev;
				free(aux_actual->value);
				free(aux_actual);
				return 0;
			}
			
			aux_actual = aux_actual->next;
			
		}

		
	}
	
	return -1;	// not in dictionary

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

			printf("READING\n"); fflush(stdout);

			// search the dictionary
			dictionary* kv_entry;
			int search_error = search_value(message.key, 'r', &kv_entry, 0);
			
			printf("after search: err=%d\n", search_error); fflush(stdout);
		
			// entry not in dictionary
			if(search_error == -1) { 
				printf("search error / not in dict\n"); fflush(stdout);
				message.error_code = -2;
				nbytes = send(socket_fd , &message, sizeof(message), 0);
				return ((int) -1);
			}
			
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
				return ((int) -1);
			}
		
		}
	
		if(message.op == 'w') {

			printf("WRITING\n"); fflush(stdout);
			
			dictionary* kv_entry = (dictionary*) malloc(sizeof(dictionary));		
			kv_entry->value = malloc(message.value_length);//allocate the necessary space for the message value

			nbytes = recv(socket_fd, kv_entry->value , message.value_length , 0);//only read up to param value_length
	 		printf("after recv\n");fflush(stdout);
			if(nbytes != message.value_length) {
				perror("receive values failed");
				return ((int) -1);
			}

			// check if given key already exists
			dictionary* search_entry;
			int search_error = search_value(kv_entry->key, 'w', &search_entry, message.overwrite);
			
			printf("end of check if given key already exists\n");fflush(stdout);
			
			
		
		}

		if(message.op == 'd') {
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

		if(message.op == 'c') {
			
			close(socket_fd);
			break;
			
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
		new_socket = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);
		if(new_socket == 0) {
			perror("socket accept");
			exit(-1);
		}

		printf("after accept sck=%d\n", new_socket);
		err = pthread_create(&tid, NULL, handle_requests, (void*) new_socket);
		if(err!=0) {
			perror("pthread_create");
			exit(-1);
		}
		
		printf("Request accepted\n");
        
	}

	
	exit(0);
    
}
