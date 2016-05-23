#ifndef __DATABASE__
#define __DATABASE__


#include "psiskv.h"

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


typedef struct dictionary_entry{
	uint32_t key;
	uint32_t value_length;
	void* value;
	struct dictionary_entry* next;
	struct dictionary_entry* prev;
}dictionary;

dictionary * find_entry(uint32_t key);
int add_entry(uint32_t key, void * value, uint32_t value_length, int overwrite );
int remove_entry(uint32_t key);
int read_entry(uint32_t key, dictionary ** entry);
#endif