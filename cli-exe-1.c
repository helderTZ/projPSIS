#include "psiskv.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>


#define MAX_VALUES 10

int main(int argc, char*argv[]){

	char linha[100];
	pid_t pid = getpid();

	int kv = kv_connect("127.0.0.1", 9999);
	if(kv==-1){
		perror("kv_connect");
		exit(-1);
	}

	printf("\nPID %d writing first values...\n", pid);fflush(stdout);
	for (uint32_t i = atoi(argv[1]); i < atoi(argv[1])+MAX_VALUES; i ++){
		sprintf(linha, "%u", i);
		printf("PID %d writing key %d with value %s\n",pid, i, linha);fflush(stdout);
		kv_write(kv, i , linha, strlen(linha)+1, 0);
	}

	printf("\nPID %d press enter to read values\n", pid);fflush(stdout);
	//getchar();
	for (uint32_t i = atoi(argv[1]); i < atoi(argv[1])+MAX_VALUES; i ++){
		if(kv_read(kv, i , linha, 100) == 0){
			printf ("PID %d key - %10u value %s\n",pid, i, linha);fflush(stdout);
		}
	}

	printf("\nPID %d press enter to delete even values\n", pid);fflush(stdout);
	//getchar();
	for (uint32_t i = atoi(argv[1]); i < atoi(argv[1])+MAX_VALUES; i +=2){		
		printf("PID %d deleting key %d\n", pid, i);fflush(stdout);
		if(kv_delete(kv, i)==0){
			printf("PID %d deleted key - %10u\n", pid, i);fflush(stdout);
		}else{
			printf("PID %d tries to delete key - %10u but does not exists\n", pid,i);fflush(stdout);
		}
	}

	printf("\nPID %d press enter to read values\n", pid);fflush(stdout);
	//getchar();
	for (uint32_t i = atoi(argv[1]); i < atoi(argv[1])+MAX_VALUES; i ++){
		if(kv_read(kv, i , linha, 100) == 0){
			printf ("PID %d key - %10u value %s\n", pid, i, linha);fflush(stdout);
		}else {
			printf("PID %d key - %10u does not exist in dictionary\n", pid, i);fflush(stdout);
		}
	}

	printf("\nPID %d writing new values...\n", pid);fflush(stdout);
	for (uint32_t i = atoi(argv[1]); i < atoi(argv[1])+MAX_VALUES; i ++){
		sprintf(linha, "%u", i*10);
		printf("PID %d writing key %d with value %s\n", pid, i, linha);fflush(stdout);
		kv_write(kv, i , linha, strlen(linha)+1, 0); /* will not overwrite*/
	}

	printf("\nPID %d press enter to read new values\n", pid);fflush(stdout);
	//getchar();
	for (uint32_t i = atoi(argv[1]); i < atoi(argv[1])+MAX_VALUES; i ++){
		if(kv_read(kv, i , linha, 100) == 0){
			printf ("PID %d key - %10u value %s\n", pid, i, linha);fflush(stdout);
		}
	}
	kv_close(kv);
	
}
