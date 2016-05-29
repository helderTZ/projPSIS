#include "psiskv.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


#define MAX_VALUES 100

int main(int argc, char*argv[]){

	char linha[100];
	pid_t pid = getpid();

	if(argc != 3)
		exit(-1);

	int kv = kv_connect("127.0.0.1", 9999);
	if(kv==-1){
		perror("kv_connect");
		exit(-1);
	}

	printf("\nPID %d press enter to write values\n", pid);fflush(stdout);
	//getchar();
	for (uint32_t i = atoi(argv[1]); i < atoi(argv[2]); i ++){
		sprintf(linha, "%u", i);
		printf("PID %d writing key %d with value %s\n",pid, i, linha);fflush(stdout);
		kv_write(kv, i , linha, strlen(linha)+1, 0);
	}
	kv_close(kv);
	
}
