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
#include <sys/ipc.h>
#include <sys/shm.h>
 
#define MSG_NOT_EXISTS -2
#define AVAILABLE 1
#define NOT_AVAILABLE 0
#define TOTAL_PORTS 20000

/*typedef struct ports {
    int port;
    char status;
}s_ports;*/


kv_client2server message_thread;
 
int read_db(int socket_fd, kv_client2server message);
int write_db(int socket_fd, kv_client2server message);
int delete_db(int socket_fd, kv_client2server message);
int close_db(int);
 
 
 
 
//TODO: transformar o log num backup periodicamente, por exemplo, no principio e no fim
/*void updateBackup() {
 
    FILE *fp = fopen(LOG_FILE, "a");
     
    fclose(fp);
 
}*/

void signal_handler(int n) {

    //updateBackup();
    
}


void error_and_die(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}


 
void * heartbeat_thread(void* arg) {
	int * shm = (int *) arg;
	while(1){
		*shm=0;
		sleep(5);
	}
}
 
void * handle_requests(void* arg) {
 
    int socket_fd = *((int*) arg);
    int nbytes;
    int aux;
    int i=0;
 
    while(1) {
         
        //DEBUG
        //printf("\n\nstart of loop, i=%d\n", i++); fflush(stdout);
         
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
            printf("Inside close op\n");
            if(close_db(socket_fd)!=0)
                error_and_die("close_db failed\n");
            break;// break out of loop if client wants to close connection
        }
         
 
    }//end while
     
    // terminate this thread
    //printf("before exit handle_requests\n");
    exit(0);
     
}//end handle_requests
 
 
 
 
 
 
int read_db(int socket_fd, kv_client2server message) {
 
    dictionary * entry;
    int err, nbytes;
 
    printList();
 
    message.error_code=read_entry(message.key, &entry);
    //printf("message error_code=%d\n", message.error_code);

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
 
    void * value;
    int err, nbytes;
 
    value = malloc(message.value_length);//allocate the necessary space for the message value
 
    nbytes = recv(socket_fd, value , message.value_length , 0);//only read up to param value_length
    if(nbytes != message.value_length) {
        perror("receive values failed");
        return -1;
    }
     
    message.error_code=add_entry(message.key, value, message.value_length, message.overwrite );
     
    nbytes = send(socket_fd, &message, sizeof(message), 0);
    if(nbytes != sizeof(message)) {
        perror("send failed");
        return -1;
    }
}
 
 
int delete_db(int socket_fd, kv_client2server message) {
    int err, nbytes;
 
    message.error_code = delete_entry(message.key);
 
    nbytes = send(socket_fd , &message, sizeof(message), 0);
    if(nbytes!=sizeof(message)) return -1;
    else return 0;
}
 
 
int close_db(int socket_fd) {
    if(create_backup("backup_teste.bin")==-1) printf("create_backup error\n");
    return close(socket_fd);
}
 
 
 
/*int initialisation() {
 
    //TODO: ler backup e preencher o dictionary
    //TODO: definir estrutura para guardar no backup, ex:
    //          uma entry:  key
    //                      value_length
    //                      value
     
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
 
}*/

 
int main(){
 
    kv_client2server m;
    int option = 1;
    int nbytes;
    struct sockaddr_in local_addr;
    struct sockaddr_in client_addr;
    int err;
    int socket_fd;
    int new_socket;
    pthread_t tid;
    int backlog;
    //s_ports available_ports[TOTAL_PORTS];

    /*//initialise ports struct
    for(int i=0; i<TOTAL_PORTS; i++) {
        available_ports[i].port = TOTAL_PORTS+1;
        available_ports[i].status = AVAILABLE;
    }*/

    //Shared Memory;
	int shmid;
	key_t key = 1234; //name of shared memory segment
	int *shm;
	int heartbeat=1;

     
    /* initialisation */
    dictionary_init();
    if(read_backup("backup_teste.bin")==-1) printf("Backup not exists\n");
    printList();
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
     
    /* create socket  */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) error_and_die("socket");
 
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(DATA_PORT);
    //err = inet_aton("127.0.0.1", &(local_addr.sin_addr));
    if(err == -1) error_and_die("inet_aton");
    local_addr.sin_addr.s_addr = INADDR_ANY;
    
    //make socket immediatly available after it closes
    setsockopt(socket_fd,SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option));

    /* bind socket */
    err = bind(socket_fd, (struct sockaddr*) &local_addr, sizeof(local_addr));
    if(err == -1) error_and_die("bind");
 
    /* listen */
    err = listen(socket_fd, backlog);
    if(err == -1) error_and_die("listen");


    err = pthread_create(&tid, NULL, heartbeat_thread,(void *) shm);
    if(err!=0) error_and_die("pthread_create heartbeat");


    int client_addr_size = sizeof(client_addr);
 
    while(1) {
        
 		
        printf("waiting for accept...\n");
        new_socket = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_size);
        if(new_socket == 0) error_and_die("socket accept");

        err = pthread_create(&tid, NULL, handle_requests, (void *) (&new_socket));
        if(err!=0)error_and_die("pthread_create");
    }
    exit(0);
     
}
