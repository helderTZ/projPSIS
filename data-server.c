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
 
kv_client2server message_thread;
 
int read_db(int socket_fd, kv_client2server message);
int write_db(int socket_fd, kv_client2server message);
int delete_db(int socket_fd, kv_client2server message);
int close_db(int);
 
 
 
 
//TODO: transformar o log num backup periodicamente, por exemplo, no principio e no fim
/*void log() {
 
    fopen(LOG_FILE, "a");
     
     
     
     
     
     
    fclose(LOG_FILE);
 
}*/
 
 
 
void * handle_requests(void* arg) {
 
    int socket_fd = *((int*) arg);
    int nbytes;
    int aux;
    int i=0;
 
 
    //DEBUG
    //printf("Hello thread world! ckt=%d\n", socket_fd); fflush(stdout);
     
    while(1) {
         
        //DEBUG
        printf("\n\nstart of loop, i=%d\n", i++); fflush(stdout);
         
        /* read message */
        nbytes = recv(socket_fd, &message_thread, sizeof(message_thread), 0);
        if(nbytes != sizeof(message_thread)) {
            perror("reveive failed");
            exit(-1);
        }
 
        //DEBUG
        printf("nbytes: %d\n", nbytes); fflush(stdout);
        printf("message:\n"); fflush(stdout);
        printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
                message_thread.op, message_thread.key, message_thread.value_length, message_thread.overwrite, message_thread.error_code); 
        fflush(stdout);
 
        printf("Received message\n"); fflush(stdout);
         
         
 
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
            close_db(socket_fd);
            // break out of loop if client wants t oclose connection
            break;
        }
         
 
    }//end while
     
    // terminate this thread
    //pthread_exit();
     
}//end handle_requests
 
 
 
 
 
 
int read_db(int socket_fd, kv_client2server message) {
 
    dictionary * entry;
    int err, nbytes;
 
    printList();
 
    //DEBUG
    printf("\n\n------------------READING--------------------\n"); fflush(stdout);
     
     
 
    message.error_code=read_entry(message.key, &entry);
 
    printf("message value_length=%d\tmessage requested_length=%d\n", 
            entry->value_length, message.value_length);
 
    if(entry->value_length > message.value_length)//message not entirely read
        message.error_code=-3;
 
    else{
        message.value_length=entry->value_length;
        //message.value=entry->value;
    }
 
    printf("message real_length=%d\n", message.value_length);
 
    printf("message inside read_db BEFORE sending:\n"); fflush(stdout);
        printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
                message.op, message.key, message.value_length, message.overwrite, message.error_code); 
        fflush(stdout);
 
         
        printf("socket_fd=%d\n", socket_fd);
    // send message header to client with size of msg
    nbytes = send(socket_fd , &message, sizeof(message), 0);
 
    printf("message inside read_db AFTER sending:\n"); fflush(stdout);
        printf("op :%c, key: %d, value_length: %d, overwrite: %d, error_code: %d\n", 
                message.op, message.key, message.value_length, message.overwrite, message.error_code); 
        fflush(stdout);
 
    nbytes = send(socket_fd, entry->value, message.value_length, 0);
    if(nbytes != message.value_length) {
        perror("send failed");
        //TODO: ver se é preciso fazer o join da thread -  ver slides
        //pthread_exit(&kv_entry->value_length);
        return -1;
    }
 
    return 0;
}
 
 
 
 
int write_db(int socket_fd, kv_client2server message) {
 
    void * value;
    int err, nbytes;
 
    //DEBUG
    printf("\n\n---------------------WRITING------------------\n"); fflush(stdout);
 
    value = malloc(message.value_length);//allocate the necessary space for the message value
 
    nbytes = recv(socket_fd, value , message.value_length , 0);//only read up to param value_length
    //printf("after recv\n");fflush(stdout);
    if(nbytes != message.value_length) {
        perror("receive values failed");
        return -1;
    }
 
    //printf("Before addentry\n");fflush(stdout);
     
    message.error_code=add_entry(message.key, value, message.value_length, message.overwrite );
     
    printf("after addentry: error_code=%d\n", message.error_code);fflush(stdout);
    nbytes = send(socket_fd, &message, sizeof(message), 0);
    if(nbytes != sizeof(message)) {
        perror("send failed");
        //TODO: ver se é preciso fazer o join da thread -  ver slides
        //pthread_exit(&kv_entry->value_length);
        return -1;
    }
 
    //DEBUG
    //printf("end of check if given key already exists\n");fflush(stdout);
 
}
 
 
int delete_db(int socket_fd, kv_client2server message) {
    int err, nbytes;
 
    message.error_code = delete_entry(message.key);
 
    nbytes = send(socket_fd , &message, sizeof(message), 0);
    if(nbytes!=sizeof(message))
        return -1;
    else return 0;
}
 
 
int close_db(int socket_fd) {
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
    int nbytes;
    struct sockaddr_in local_addr;
    struct sockaddr_in client_addr;
    int err;
    int socket_fd;
    int new_socket;
    pthread_t tid;
    int backlog;
     
     
    /* initialisation */
    //initialisation();
    dictionary_init();
     
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
        err = pthread_create(&tid, NULL, handle_requests, (void *) (&new_socket));
        if(err!=0) {
            perror("pthread_create");
            exit(-1);
        }
         
        printf("Request accepted\n");
         
    }
 
     
    exit(0);
     
}
