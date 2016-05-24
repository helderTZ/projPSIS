#include "psiskv.h"
#include "data-server.h"
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



//TODO: tratar do CTRL+C


int main(int argc, char *argv[], char *envp[]){

    kv_client2server m;
	int nbytes;
	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	int err;
	int socket_fd;
	int new_socket;
	pthread_t tid;
	int backlog;

	system("pwd > pwd.txt");
	FILE *f_pwd = fopen("pwd.txt", "r");
	char pwd[100];
	fscanf(f_pwd, "%s", pwd);
	sprintf(pwd, "%s/data-server", pwd);
	printf("pwd: %s\n", pwd);
	fclose(f_pwd);

	err=fork();
	if (err==0){
		char* newarg[] = {pwd, "data-server", NULL};
		execve("data-server", newarg, envp);
    }else {

    	printf("blahblahblah\n");

    	/*
	
		// create socket
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
		

		// bind socket
		err = bind(socket_fd, (struct sockaddr*) &local_addr, sizeof(local_addr));
		if(err == -1) {
			perror("bind");
			exit(-1);
		}

		// listen
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

		}*/

	}

	
	exit(0);
    
}
