#include "psiskv.h"
#include <sys/socket.h>
#include <sys/types.h>

int kv_connect(char * kv_server_ip, int kv_server_port) {
	int option = 1;
	int socket_fd;
	struct sockaddr_in server_addr;
	int dataserver_port;

	/* open socket */
	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0) )== -1) {
		perror("Creating socket");
		return -1;
	}

	setsockopt(socket_fd,SOL_SOCKET,(SO_REUSEADDR),(char*)&option,sizeof(option));

	/* fill struct */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(kv_server_port);
	if((inet_aton(kv_server_ip, &(server_addr.sin_addr))) == 0)  {
 		perror("inet_aton");
		return -1;
	}
	
	/* connect */
	int err = connect(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));
	if (err == -1){
		perror("connect 1");
		return -1;
	}

	//receive data-server's port from front-server
	int nbytes = recv(socket_fd, &dataserver_port, sizeof(int), 0);
	if(nbytes != sizeof(int)) {
		perror("kv_connect: receive port failed");
		return -1;
	}

	//closing front-server's  connection
	if(close(socket_fd) == -1) {
		perror("closing socket");
	}

	/* open socket */
	if((socket_fd = socket(AF_INET, SOCK_STREAM, 0) )== -1) {
		perror("Creating socket");
		return -1;
	}

	setsockopt(socket_fd,SOL_SOCKET,(SO_REUSEADDR),(char*)&option,sizeof(option));

	/* fill struct */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(dataserver_port);
	if((inet_aton(kv_server_ip, &(server_addr.sin_addr))) == 0)  {
 		perror("inet_aton");
		return -1;
	}

	printf("socket=%d, port=%d\n", socket_fd, dataserver_port); fflush(stdout);

	/* connect */
	err = connect(socket_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr));
	if (err == -1){
		perror("connect 2");
		return -1;
	}

	return socket_fd;

}


int kv_write(int kv_descriptor, uint32_t key, char * value, int value_length, int kv_overwrite) {

	//printf("\n\n--------------------WRITING---------------\n"); fflush(stdout);

	kv_client2server message;
	message.op = 'w';	// para identificar ao server que é uma operação de write
	message.key = key;
	message.value_length = value_length;
	message.overwrite = kv_overwrite;
	message.error_code = 0;

	/*printf("message:\n"); fflush(stdout);
	printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
			message.op, message.key, message.value_length, message.overwrite, message.error_code); 
	fflush(stdout);*/

	/* send message header */
    int nbytes = send(kv_descriptor, &message, sizeof(message), 0);
	
	/* send message */
    nbytes = send(kv_descriptor, value, value_length, 0);
	if(nbytes != value_length) {
		perror("send failed");
		return -1;
	}
	
	nbytes = recv(kv_descriptor, &message, sizeof(message), 0);
	if(nbytes != sizeof(message)) {
		perror("kv_write: receive error_code of msg failed");
		return -1;
	}

	return 0;
	
}


int kv_read(int kv_descriptor, uint32_t key, char * value, int value_length) {

	//printf("\n\n--------------------READING---------------\n"); fflush(stdout);

	kv_client2server message;
	message.op = 'r';	// para identificar ao server que é uma operação de write
	message.key = key;
	message.value_length = value_length;//quanto se quer ler desta key
	message.overwrite = 0;
	message.error_code = 0;
	
	/*printf("message inside kv_read BEFORE sending:\n"); fflush(stdout);
	printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
			message.op, message.key, message.value_length, message.overwrite, message.error_code); 
	fflush(stdout);*/

    int nbytes = send(kv_descriptor, &message, sizeof(message), 0);
	
	if(nbytes != sizeof(message)) {
		perror("send failed");
		return -1;
	}

	//message_recv=(kv_client2server *) malloc(sizeof(kv_client2server));

	/*printf("message inside kv_read BEFORE receiving size of value:\n"); fflush(stdout);
	printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
			message.op, message.key, message.value_length, message.overwrite, message.error_code); 
	fflush(stdout);*/

	//read size of message and error message
	nbytes = recv(kv_descriptor, &message, sizeof(message), 0);
	if(nbytes != sizeof(message)) {
		perror("receive size of msg failed");
		return -1;
	}
	
	/*printf("message inside kv_read AFTER receiving size of value:\n"); fflush(stdout);
	printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
			message.op, message.key, message.value_length, message.overwrite, message.error_code); 
	fflush(stdout);*/
	
	//If the values does not exist in dictionary
	if(message.error_code == -2){
		//perror("value does not exist in dictionary\n");
		return -2;
	}

	/* read message */
	nbytes = recv(kv_descriptor, (void *)value , message.value_length, 0);//only read up to param value_length
	
	//check length received
	if(nbytes != message.value_length) {
		perror("receive values failed");
		return -1;
	}

	//printf("message value=%s\n", value); fflush(stdout);

	//free(message_recv);
	return 0;

	
}

int kv_close(int kv_descriptor) {

	kv_client2server message;
	message.op = 'c';	// close operation
	message.key = 0;
	message.value_length = 0;//quanto se quer ler desta key
	message.overwrite = 0;
	message.error_code = 0;

	int nbytes = send(kv_descriptor, &message, sizeof(message), 0);
	
	if(nbytes != sizeof(message)) {
		perror("kv_close: send failed");
		return -1;
	}

	if(close(kv_descriptor) == -1) {
		perror("closing socket");
	}

}

int kv_delete(int kv_descriptor, uint32_t key) {
	
	kv_client2server message;
	message.op='d';
	message.key=key;
	message.value_length = 0;
	message.overwrite = 0;
	message.error_code = 0;
	
	//send order
	int nbytes = send(kv_descriptor, &message, sizeof(message), 0);
	
	if(nbytes != sizeof(message)) {
		perror("send failed");
		return -1;
	}
	
	//check if it was deleted
	nbytes = recv(kv_descriptor, &message, sizeof(kv_client2server), 0);

	if(message.error_code != 0) {
		perror("delete failed");
		return -1;
	}

	return 0;	
}

