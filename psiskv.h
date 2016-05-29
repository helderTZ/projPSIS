#ifndef PSISKV_H
#define PSISKV_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define FRONT_PORT 9999
#define DATA_PORT 20000
#define LOG_FILE "/tmp/log.txt"
#define BACKUP_FILE "/tmp/backup.bin"

/* From client to server */

typedef struct kv_message {
	char op;
	uint32_t key;
	int value_length;
	char overwrite;
	char error_code;
}kv_message;


int kv_connect(char * kv_server_ip, int kv_server_port);
int kv_close(int kv_descriptor);
int kv_delete(int kv_descriptor, uint32_t key);
int kv_write(int kv_descriptor, uint32_t key, char * value, int value_length, int kv_overwrite);
int kv_read(int kv_descriptor, uint32_t key, char * value, int value_length);

#endif
