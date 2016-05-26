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

struct pii {
	int* shm;
	char** envp;
};

void* manage_client(void *arg) {

	int socket_fd = *((int*) arg);
	int dataserver_port = DATA_PORT;

	//send data-server's port number to client
	int nbytes = send(socket_fd , &dataserver_port, sizeof(int), 0);
	if(nbytes!=sizeof(int)) {
		perror("sending port number to client");
		return;
	}

}

void wake_dataserver(char** envp) {

	// Required for data-server
	system("pwd > pwd.txt");
	FILE *f_pwd = fopen("pwd.txt", "r");
	char pwd[100];
	fscanf(f_pwd, "%s", pwd);
	sprintf(pwd, "%s/data-server", pwd);
	printf("pwd: %s\n", pwd);
	fclose(f_pwd);

	int pid = fork();
	if (pid == 0){
		char* newarg[] = {pwd, "data-server", NULL};
		execve("data-server", newarg, envp);
	}

}


void* manage_heartbeat(void *arg) {

	struct pii args = *(struct pii*)arg;
	int  *shm 	= args.shm;
	char **envp = args.envp;

	int heartbeat=1;
	while(heartbeat){
		*shm=1;
		sleep(10);
		if(*shm==1)
			heartbeat=0;
		else{
			wait();
			wake_dataserver( envp );
		}
	}

}


void error_and_die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}


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
	pid_t pid;

	// Shared Memory;
	int shmid;
    key_t key;
    int *shm;

    //Socket stuff
    int option=1;
    int dataserver_port = DATA_PORT;


	//----------------------- Shared memory init --------------------

	//We'll name our shared memory segment "1234".
    key = 1234;

    //Create the segment.
    if ((shmid = shmget(key, sizeof(int), IPC_CREAT | 0666)) < 0) error_and_die("shmget-front server");

    //Now we attach the segment to our data space.
    if ((shm = shmat(shmid, NULL, 0)) == (int *) -1) error_and_die("shmat");



    //----------------------- launch data-server --------------------
    wake_dataserver(envp);



    //--------------------------- heartbeat -------------------------
    struct pii args;
    args.shm = shm;
    args.envp = envp;
	pthread_create( &tid, NULL, manage_heartbeat, (void *)(&args) );





	//----------------------- Socket creation -----------------------

	// create socket
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(FRONT_PORT);
	//err = inet_aton("127.0.0.1", &(local_addr.sin_addr));
	if(err == -1) {
		perror("inet_aton");
		exit(-1);
	}
	local_addr.sin_addr.s_addr = INADDR_ANY;
	
	//make socket immediatly available after it closes
    setsockopt(socket_fd,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option));

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


	int local_addr_size = sizeof(local_addr);
	int client_addr_size = sizeof(client_addr);


  	while(1){

    	//----------------------- manage client connections ---------------
  		
		printf("before accept\n");
		new_socket = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);
		if(new_socket == 0) {
			perror("socket accept");
			exit(-1);
		}

		printf("after accept sck=%d\n", new_socket);
		err = pthread_create(&tid, NULL, manage_client, (void *) (&new_socket));
		if(err!=0) {
			perror("pthread_create");
			exit(-1);
		}
	}
}