#ifndef DATASERVER_H
#define DATASERVER_H

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

#define KV_FIFO "/tmp/kv_fifo"

#define AVAILABLE 1
#define NOT_AVAILABLE 0
#define INITIAL_PORT 20000
#define TOTAL_PORTS 1000

kv_client2server message_thread;


int read_db(int socket_fd, kv_client2server message);
int write_db(int socket_fd, kv_client2server message);
int delete_db(int socket_fd, kv_client2server message);
int close_db(int);
void * handle_requests(void* arg);


#endif