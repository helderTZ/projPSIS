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
#endif

typedef struct dictionary_entry{
	uint32_t key;
	uint32_t value_length;
	void* value;
	struct dictionary_entry* next;
	struct dictionary_entry* prev;
}dictionary;

