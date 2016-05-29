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

	printf("\nPID %d press enter to delete even values\n", pid);fflush(stdout);
	//getchar();
	for (uint32_t i = atoi(argv[1]); i < atoi(argv[2]); i += 2){		
		printf("PID %d deleting key %d\n", pid, i);fflush(stdout);
		if(kv_delete(kv, i)==0){
			printf("PID %d deleted key - %10u\n", pid, i);fflush(stdout);
		}else{
			printf("PID %d tries to delete key - %10u but does not exists\n", pid,i);fflush(stdout);
		}
	}
	kv_close(kv);
	
}
