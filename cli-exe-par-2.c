#include "psiskv.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_VALUES 5
#define FORKS 1

int main(){
	char linha[100];
	int i;	
	
	for(i=0; i<FORKS; i++){
	
		if(fork() == 0){
			
			int kv = kv_connect("127.0.0.1", 9999);
			printf("CLIENT 0 connected to data-server\n"); fflush(stdout);

			for (uint32_t i = 0; i < MAX_VALUES; i +=2){
				sprintf(linha, "%u", i);
				printf("CLIENT 0 writing key %d with value %s\n", i, linha); fflush(stdout);
				kv_write(kv, i , linha, strlen(linha)+1, 0);
			}
			kv_close(kv);


		}else{
			int kv = kv_connect("127.0.0.1", 9999);
			printf("CLIENT 1 connected to data-server\n"); fflush(stdout);

			for (uint32_t i = 1; i < MAX_VALUES; i +=2){
				sprintf(linha, "%u", i);
				printf("CLIENT 1 writing key %d with value %s\n", i, linha); fflush(stdout);
				kv_write(kv, i , linha, strlen(linha)+1, 0);
			}


			wait(NULL);
			printf("writing values\n");
			for (uint32_t i = 1; i < MAX_VALUES; i +=2){
				sprintf(linha, "%u", i);
				printf("CLIENT 1 writing key %d with value %s\n", i, linha); fflush(stdout);
				kv_write(kv, i , linha, strlen(linha)+1, 0);
			}
			kv_close(kv);
		}
	}
}
