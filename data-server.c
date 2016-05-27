#include "psiskv.h"
#include "data-server.h"
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
#include <sys/ipc.h>
#include <sys/shm.h>
 
#define MSG_NOT_EXISTS -2

typedef struct ports {
    int port;
    char status;
}s_ports;

kv_client2server message_thread;
extern pthread_mutex_t mutex;
s_ports available_ports[TOTAL_PORTS];
 
int read_db(int socket_fd, kv_client2server message);
int write_db(int socket_fd, kv_client2server message);
int delete_db(int socket_fd, kv_client2server message);
int close_db(int);
 
 
 
 
//TODO: transformar o log num backup periodicamente, por exemplo, no principio e no fim
/*void updateBackup() {
 
    FILE *fp = fopen(LOG_FILE, "a");
     
    fclose(fp);
 
}*/

void error_and_die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

void signal_handler(int n) {

    if(!close_log()) error_and_die("closing log_file\n");
    //updateBackup();
    
}

void * heartbeat_thread(void* arg) {
	int * shm = (int *) arg;
	while(1){
		*shm=0;
		sleep(5);
	}
}
 
void * handle_requests(void* arg) {
 
    int socket_fd = (int) arg;
    int nbytes;
    int aux;
    int i=0;
    
    printf("entered handle_requests socket = %d\n", socket_fd); fflush(stdout);
 
    while(1) {
         
        /* read message */
        nbytes = recv(socket_fd, &message_thread, sizeof(message_thread), 0);
        if(nbytes != sizeof(message_thread)) {
            perror("reveive failed");
            exit(-1);
        }
 
        //DEBUG
        /*printf("nbytes: %d\n", nbytes); fflush(stdout);
        printf("message:\n"); fflush(stdout);
        printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
                message_thread.op, message_thread.key, message_thread.value_length, message_thread.overwrite, message_thread.error_code); 
        fflush(stdout);*/
 
        //printf("Received message\n"); fflush(stdout);
        
        

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
            if(close_db(socket_fd)!=0)
                error_and_die("close_db failed\n");
            break;// break out of loop if client wants to close connection
        }
         
 
    }//end while
     
    // terminate this thread
    printf("exiting handle_requests\n"); fflush(stdout);
    return NULL;
     
}//end handle_requests
 
 
int read_db(int socket_fd, kv_client2server message) {

    printf("---------------------------- READING --------------------\n"); fflush(stdout);
 
    dictionary * entry;
    int err, nbytes;

    message.error_code=read_entry(message.key, &entry);

    if (message.error_code!=MSG_NOT_EXISTS)
        if(entry->value_length > message.value_length)//message not entirely read
            message.error_code=-3; 
        else{
            message.value_length=entry->value_length;
        }
    // send message header to client with real size of msg
    nbytes = send(socket_fd , &message, sizeof(message), 0);

    if(message.error_code!=MSG_NOT_EXISTS){//only send message if it exists
        nbytes = send(socket_fd, entry->value, message.value_length, 0);
        if(nbytes != message.value_length) {
            perror("read_db: send failed");
            return -1;
        }
    }
    return 0;
}
 
 
 
 
int write_db(int socket_fd, kv_client2server message) {

    printf("---------------------------- WRITING --------------------\n"); fflush(stdout);
 
    void * value;
    int err, nbytes;
 
    value = malloc(message.value_length);//allocate the necessary space for the message value
 
    nbytes = recv(socket_fd, value , message.value_length , 0);//only read up to param value_length
    if(nbytes != message.value_length) {
        perror("receive values failed");
        return -1;
    }

    printf("received value = %s\n", (char*)value); fflush(stdout);
     
    message.error_code = add_entry(message.key, value, message.value_length, message.overwrite );


    printf("added key = %d value = %s error_code = %d\n", message.key, (char*)value, message.error_code); fflush(stdout);
     
    nbytes = send(socket_fd, &message, sizeof(message), 0);
    if(nbytes != sizeof(message)) {
        perror("send failed");
        return -1;
    }
}
 
 
int delete_db(int socket_fd, kv_client2server message) {

    printf("---------------------------- DELETING --------------------\n"); fflush(stdout);

    int err, nbytes;
 
    message.error_code = delete_entry(message.key);
 
    nbytes = send(socket_fd , &message, sizeof(message), 0);
    if(nbytes!=sizeof(message)) return -1;
    else return 0;
}
 

int close_db(int socket_fd) {

    printf("---------------------------- CLOSING --------------------\n"); fflush(stdout);
    printf("closing socket=%d\n", socket_fd); fflush(stdout);

    //available_ports[socket_fd-INITIAL_PORT].status = AVAILABLE;
    //if(create_backup("backup_teste.bin")==-1) printf("create_backup error\n");
    return close(socket_fd);
}
 
int main(){
 
    kv_client2server m;
    int option = 1;
    int nbytes;
    struct sockaddr_in local_addr;
    struct sockaddr_in client_addr;
    int client_addr_size;
    int err;
    int i;
    int socket_fd;
    int new_socket;
    pthread_t tid;
    int backlog = 100;

    /*//initialise ports struct
    for(i=0; i<TOTAL_PORTS; i++) {
        available_ports[i].port = TOTAL_PORTS+1;
        available_ports[i].status = AVAILABLE;
    }*/

    //Shared Memory;
	int shmid;
	key_t key = 1234; //name of shared memory segment
	int *shm;
	int heartbeat=1;

    // Mutex
    if(pthread_mutex_init(&mutex, NULL) != 0){
        printf("mutex creation error\n");
        exit(-1);
    }
     
    /* initialisation */
    dictionary_init();
    //if(read_backup("backup_teste.bin") < 0) printf("Backup not exists\n");
    printf("after reading backup\n");
    //if(log_init ("log_file.bin", "a+") < 0) printf("Error opening log_file\n");
    //if(read_log() < 0) printf("Error executing or empty log_file\n");
    //if(log_init ("log_file.bin", "w+") < 0) printf("Error opening log_file\n");
	

    /* set handler for signal SIGINT */
    struct sigaction new_action;
    new_action.sa_handler = signal_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction (SIGINT, &new_action, NULL);
     
    
    //Shared memory init***************************
    /*
     * Create the segment.
     */
    if ((shmid = shmget(key, sizeof(int), IPC_CREAT | 0666)) < 0) error_and_die("shmget-front server");

    /*
     * Now we attach the segment to our data space.
     */
    if ((shm = shmat(shmid, NULL, 0)) == (int *) -1) error_and_die("shmat");
    //********************************************************
    



    //------------------------ Create FIFO -------------------------
    //int fifo;
    //fifo = open(KV_FIFO, O_WRONLY );    // open fifo for writing


    //while(1) {

        /*// search for available socket
        for(i=0; i<TOTAL_PORTS; i++) {
            if(available_ports[i].status == AVAILABLE) {
                socket_fd = available_ports[i].port;
                available_ports[i].status == NOT_AVAILABLE;
                break;
            }
        }*/

        //write(fifo, &socket_fd, sizeof(int));



        /* create socket  */
        if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) error_and_die("socket");
     
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(INITIAL_PORT);
        //err = inet_aton("127.0.0.1", &(local_addr.sin_addr));
        if(err == -1) error_and_die("inet_aton");
        local_addr.sin_addr.s_addr = INADDR_ANY;
        
        //make socket immediatly available after it closes
        setsockopt(socket_fd,SOL_SOCKET,(SO_REUSEADDR),(char*)&option,sizeof(option));

        /* bind socket */
        err = bind(socket_fd, (struct sockaddr*) &local_addr, sizeof(local_addr));
        if(err == -1) error_and_die("bind");
     
        /* listen */
        err = listen(socket_fd, backlog);
        if(err == -1) error_and_die("listen");


        err = pthread_create(&tid, NULL, heartbeat_thread,(void *) shm);
        if(err!=0) error_and_die("pthread_create heartbeat");


        client_addr_size = sizeof(client_addr);
        int dummy_socket;
 
    while(1){
 		
        printf("DATA-SERVER: waiting for accept...\n"); fflush(stdout);
        new_socket = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);
        if(new_socket == 0) error_and_die("socket accept");

		printf("DATA-SERVER: accepted\n"); fflush(stdout);
        //printf("socket=%d\n", new_socket); fflush(stdout);
        
        dummy_socket = new_socket;

        err = pthread_create(&tid, NULL, handle_requests, (void *) new_socket);
        if(err!=0)error_and_die("pthread_create");
    }

    //pthread_mutex_destroy(&mutex);
    exit(0);
     
}
