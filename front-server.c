#include "data-server.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

int dataserver_port = INITIAL_PORT;
int pid;




void* manage_operator(void* arg) {
	char command[100];

	while(1) {
		scanf("%s", command);
		if(strcmp(command, "quit") == 0) {
			printf("Terminating...\n");fflush(stdout);
			kill(pid, SIGKILL);
			exit(0);
		}
	}
}

void signal_handler(int n) {

    kill(pid, SIGKILL);
	exit(0);
    
}

void* manage_client(void *arg) {

	int socket_fd = (int) arg;

	//send data-server's port number to client
	int nbytes = send(socket_fd , &dataserver_port, sizeof(int), 0);
	if(nbytes!=sizeof(int)) {
		perror("sending port number to client");
		return NULL;
	}

	return NULL;

}

void wake_dataserver(char** envp) {

	// Required for data-server
	system("pwd > pwd.txt");
	FILE *f_pwd = fopen("pwd.txt", "r");
	char pwd[100];
	fscanf(f_pwd, "%s", pwd);
	sprintf(pwd, "%s/data-server", pwd);
	fclose(f_pwd);

	pid = fork();
	if (pid == 0){
		char* newarg[] = {pwd, "data-server", NULL};
		execve("data-server", newarg, envp);
	}

}


/*void* manage_heartbeat(void *arg) {

	struct pii args = *(struct pii*)arg;
	int  *shm 	= args.shm;
	char **envp = args.envp;
	int status;

	int heartbeat=1;
	while(heartbeat){
		*shm=1;
		sleep(10);
		if(*shm==1)
			heartbeat=0;
		else{
			wait(&status);
			wake_dataserver( envp );
		}
	}

}*/

void* manage_heartbeat(void *arg) {

	struct pii args = *(struct pii*)arg;
	int  *shm 	= args.shm;
	char **envp = args.envp;
	int status;

	while(1){
		*shm=1;
		sleep(2);
		if(*shm==1) {
			printf("reviving data-server *shm=%d\n", *shm); fflush(stdout);
			wait(&status);
			wake_dataserver( envp );
		}
	}

}


void error_and_die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}


int main(int argc, char *argv[], char *envp[]){

	struct sockaddr_in local_addr;
	struct sockaddr_in client_addr;
	int err;
	int socket_fd;
	int new_socket;

	//Thread stuff
	pthread_t tid[MAX_CLIENTS];
	int i=0;
	pthread_t tid_operator, tid_heartbeat; 

	int backlog = 100;

	// Shared Memory;
	int shmid;
    key_t key;
    int *shm;

    //Socket stuff
    int option=1;


    /* set handler for signal SIGINT */
    struct sigaction new_action;
    new_action.sa_handler = signal_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction (SIGINT, &new_action, NULL);


	//----------------------- Shared memory init --------------------

	//We'll name our shared memory segment "1234".
    key = 1234;

    //Create the segment.
    if ((shmid = shmget(key, sizeof(int), IPC_CREAT | 0666)) < 0) 
    	error_and_die("shmget-front server");

    //Now we attach the segment to our data space.
    if ((shm = shmat(shmid, NULL, 0)) == (int *) -1) 
    	error_and_die("shmat");



    //----------------------- launch data-server --------------------
    wake_dataserver(envp);



    //--------------------------- heartbeat -------------------------
    struct pii args;
    args.shm = shm;
    args.envp = envp;
	pthread_create( &tid_heartbeat, NULL, manage_heartbeat, (void *)(&args) );



	/*
	//------------------------ Create FIFO -------------------------
    int fifo;
    unlink(KV_FIFO);	// delete fifo if it already exists
    mkfifo(KV_FIFO, S_IFIFO | 0666);	// create fifo file
    fifo = open(KV_FIFO, O_RDONLY );	// open fifo for reading
	*/



	//----------------------- Socket creation -----------------------

	// create socket
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(FRONT_PORT);
	//err = inet_aton("127.0.0.1", &(local_addr.sin_addr));
	local_addr.sin_addr.s_addr = INADDR_ANY;
	
	//make socket immediatly available after it closes
    setsockopt(socket_fd,SOL_SOCKET,(SO_REUSEADDR),(char*)&option,sizeof(option));

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


	//---------------------------------- operator ----------------------------------
	pthread_create(&tid_operator, NULL, manage_operator, NULL);


	socklen_t local_addr_size = sizeof(local_addr);
	socklen_t client_addr_size = sizeof(client_addr);


	int errsv;

  	while(1){

    	//----------------------- manage client connections ---------------

    	//read(fifo, &dataserver_port, sizeof(int));

		printf("FRONT-SERVER: before accept\n");fflush(stdout);
		new_socket = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);
		errsv = errno;

        //if signal is received while in accept, it will return -1 and generate the EINTR error
        if(new_socket < 0 && errsv==EINTR) continue;
		if(new_socket == 0) {
			perror("socket accept");
			exit(-1);
		}

		if(i<MAX_CLIENTS) i++;
		else i=0;

		printf("FRONT-SERVER: after accept sck=%d\n", new_socket);fflush(stdout);
		err = pthread_create(&tid[i], NULL, manage_client, (void *) new_socket);
		if(err!=0) {
			perror("pthread_create");
			exit(-1);
		}
	}
}
