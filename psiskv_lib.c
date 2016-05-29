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

	kv_message message;
	message.op = 'w';	// para identificar ao server que é uma operação de write
	message.key = key;
	message.value_length = value_length;
	message.overwrite = kv_overwrite;
	message.error_code = 0;

	/* send message header */
    int nbytes = send(kv_descriptor, &message, sizeof(kv_message), 0);
	
	/* send message */
    nbytes = send(kv_descriptor, value, value_length, 0);
	if(nbytes != value_length) {
		perror("send failed");
		return -1;
	}
	
	nbytes = recv(kv_descriptor, &message, sizeof(kv_message),MSG_WAITALL );
	if(nbytes != sizeof(kv_message)) {
		perror("kv_write: receive error_code of msg failed");
		return -1;
	}

	return 0;
	
}


int kv_read(int kv_descriptor, uint32_t key, char * value, int value_length) {

	//printf("\n\n--------------------READING---------------\n"); fflush(stdout);

	kv_message message;
	message.op = 'r';	// para identificar ao server que é uma operação de read
	message.key = key;
	message.value_length = value_length;//quanto se quer ler desta key
	message.overwrite = 0;
	message.error_code = 0;
	
    int nbytes = send(kv_descriptor, &message, sizeof(kv_message), 0);
	
	if(nbytes != sizeof(kv_message)) {
		perror("send failed");
		return -1;
	}

	//read size of message and error message
	nbytes = recv(kv_descriptor, &message, sizeof(kv_message),MSG_WAITALL);
	if(nbytes != sizeof(kv_message)) {
		perror("receive size of msg failed");
		return -1;
	}
	
	
	//If the values does not exist in dictionary
	if(message.error_code == -2){
		return -2;
	}

	/* read message */
	nbytes = recv(kv_descriptor, (void *)value , message.value_length, MSG_WAITALL); //only read up to param value_length
	
	//check length received
	if(nbytes != message.value_length) {
		perror("receive values failed");
		return -1;
	}

	return 0;

	
}

int kv_close(int kv_descriptor) {

	kv_message message;
	message.op = 'c';	// close operation
	message.key = 0;
	message.value_length = 0;//quanto se quer ler desta key
	message.overwrite = 0;
	message.error_code = 0;

	int nbytes = send(kv_descriptor, &message, sizeof(kv_message), 0);
	
	if(nbytes != sizeof(kv_message)) {
		perror("kv_close: send failed");
		return -1;
	}

	if(close(kv_descriptor) == -1) {
		perror("closing socket");
	}

	return 0;

}

int kv_delete(int kv_descriptor, uint32_t key) {
	
	kv_message message;
	message.op='d';
	message.key=key;
	message.value_length = 0;
	message.overwrite = 0;
	message.error_code = 0;
	
	//send order
	int nbytes = send(kv_descriptor, &message, sizeof(kv_message), 0);
	
	if(nbytes != sizeof(kv_message)) {
		perror("send failed");
		return -1;
	}
	
	//check if it was deleted
	nbytes = recv(kv_descriptor, &message, sizeof(kv_message),MSG_WAITALL );

	//Try to delete - Entry not exists
	if(message.error_code == -1) {
		return -1;
	}

	return 0;	
}

