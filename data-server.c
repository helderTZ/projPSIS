#include "psiskv.h"
#include "data-server.h"
#include "database.h"
 
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
#include <sys/ipc.h>
#include <sys/shm.h>
 
#define MSG_NOT_EXISTS -2


extern pthread_mutex_t mutex;
extern FILE *log_fp;
int pid;
 
int read_db(int socket_fd, kv_message message);
int write_db(int socket_fd, kv_message message);
int delete_db(int socket_fd, kv_message message);
int close_db(int);
 

void error_and_die(const char *msg) {
  perror(msg);
}


void signal_handler(int n) {

    //if(create_backup("backup_teste.bin")==-1) printf("create_backup error\n");
    if(close_log()) error_and_die("closing log_file\n");
    exit(0);
    //updateBackup();
    
}


void * manage_heartbeat(void* arg) {
	
	struct pii args = *(struct pii*)arg;
	int  *shm 	= args.shm;

	while(1){
		*shm=0;
		sleep(1);
	}

}

 
void * handle_requests(void* arg) {
 
    int socket_fd = (int) arg;
    int nbytes;
    kv_message message_thread;


    while(1) {
         
        /* read message */
        nbytes = recv(socket_fd, &message_thread, sizeof(kv_message), 0);
        if(nbytes != sizeof(message_thread)) {
            perror("reveive failed");
            exit(-1);
        }
 
        if(message_thread.op == 'r') {
            read_db(socket_fd, message_thread);
        }
     
        if(message_thread.op == 'w') {
            write_db(socket_fd, message_thread);
        }
 
        if(message_thread.op == 'd') {
            delete_db(socket_fd, message_thread);
        }
 
        if(message_thread.op == 'c') {
            if(close_db(socket_fd) !=0 )
                error_and_die("close_db failed\n");

            // break out of loop if client wants to close connection
            break;
        }
         
 
    }//end while

    // terminate this thread
    return NULL;
     
}//end handle_requests
 
 
int read_db(int socket_fd, kv_message message) {

    //printf("---------------------------- READING --------------------\n"); fflush(stdout);
 
    dictionary * entry;
    int nbytes;

    message.error_code=read_entry(message.key, &entry);

    if (message.error_code!=MSG_NOT_EXISTS) {
        if(entry->value_length > message.value_length) //message not entirely read
            message.error_code=-3; 
        else{
            message.value_length=entry->value_length;
        }
    }

    // send message header to client with real size of msg
    nbytes = send(socket_fd , &message, sizeof(message), 0);

    if(message.error_code!=MSG_NOT_EXISTS){ //only send message if it exists
        nbytes = send(socket_fd, entry->value, message.value_length, 0);
        if(nbytes != message.value_length) {
            perror("read_db: send failed");
            return -1;
        }
    }
    return 0;
}
 
 
 
 
int write_db(int socket_fd, kv_message message) {

	//printf("---------------------------- WRITING --------------------\n"); fflush(stdout);
 
    void * value;
    int nbytes;
 
    value = malloc(message.value_length); //allocate the necessary space for the message value
 
    nbytes = recv(socket_fd, value , message.value_length , 0); //only read up to param value_length
    if(nbytes != message.value_length) {
        perror("receive values failed");
        return -1;
    }
     
    message.error_code = add_entry(message.key, value, message.value_length, message.overwrite );
    
    nbytes = send(socket_fd, &message, sizeof(message), 0);
    if(nbytes != sizeof(message)) {
        perror("send failed");
        return -1;
    }

    return 0;
}
 
 
int delete_db(int socket_fd, kv_message message) {

	//printf("---------------------------- DELETE --------------------\n"); fflush(stdout);

    int nbytes;
 
    message.error_code = delete_entry(message.key);
 
    nbytes = send(socket_fd , &message, sizeof(message), 0);
    if(nbytes!=sizeof(message)) return -1;
    else return 0;
}
 

int close_db(int socket_fd) {

	//printf("---------------------------- CLOSE --------------------\n"); fflush(stdout);

    return close(socket_fd);
}
 
int main(int argc, char *argv[], char *envp[]){
 
    
    int err;
    int backlog = 100;
    
    //socket stuff
    int option = 1;
    struct sockaddr_in local_addr;
    struct sockaddr_in client_addr;
    int socket_fd;
    int new_socket;

    //threads stuff
    pthread_t tid_heartbeat;
    pthread_t tid;

    //Shared Memory;
	int shmid;
	key_t key = 1234; //name of shared memory segment
	int *shm;



    // Mutex
    if(pthread_mutex_init(&mutex, NULL) != 0){
        printf("mutex creation error\n");
        exit(-1);
    }
     
    /* initialisation */
    dictionary_init();

    #ifdef ENABLE_LOGS

    if(read_backup("backup_teste.bin") < 0) perror("Backup not exists");
    if(log_init ("log_file.bin", "a+") < 0) perror("Error opening log_file");
    if(read_log() < 0) perror("Error executing or empty log_file");
    if(create_backup("backup_teste.bin")==-1) perror("create_backup error");
    if(log_init ("log_file.bin", "w+") < 0) perror("Error opening log_file");

	#endif

    /* set handler for signal SIGINT */
    struct sigaction new_action;
    new_action.sa_handler = signal_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction (SIGINT, &new_action, NULL);




    
    //-------------------------------- Shared memory init //--------------------------------
    /*
     * Create the segment.
     */
    if ((shmid = shmget(key, sizeof(int), IPC_CREAT | 0666)) < 0) 
    	error_and_die("shmget-front server");

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (int *) -1) 
    	error_and_die("shmat");
  


    //----------------------------- Socket creation ------------------------

    /* create socket  */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    	error_and_die("socket");
 
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(INITIAL_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    
    //make socket immediatly available after it closes
    setsockopt(socket_fd,SOL_SOCKET,(SO_REUSEADDR),(char*)&option,sizeof(option));

    /* bind socket */
    err = bind(socket_fd, (struct sockaddr*) &local_addr, sizeof(local_addr));
    if(err == -1) error_and_die("bind");
 
    /* listen */
    err = listen(socket_fd, backlog);
    if(err == -1) error_and_die("listen");




    //--------------------------- heartbeat -------------------------
    struct pii args;
	args.shm = shm;
	args.envp = envp;
    pthread_create(&tid_heartbeat, NULL, manage_heartbeat,(void *)(&args) );



    socklen_t client_addr_size = sizeof(client_addr);

    // variable used to trap error
    int errsv;



 
    while(1){
 		
        new_socket = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);
        errsv = errno;

        //if signal is received while in accept, it will return -1 and generate the EINTR error
        if(new_socket < 0 && errsv==EINTR) signal_handler(0);
        if(new_socket == 0) {
        	error_and_die("socket accept");
        	continue;
        }

        err = pthread_create(&tid, NULL, handle_requests, (void *) new_socket );
        if(err!=0) {
			perror("pthread_create");
		}
    }

    exit(0);
     
}
