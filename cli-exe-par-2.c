#include "psiskv.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_VALUES 10
#define FORKS 5

int main(){
	char linha[100];
	int j;	
	
	for(j=0; j<FORKS; j++){
	
		if(fork() == 0){
			
			int kv = kv_connect("127.0.0.1", 9999);
			printf("connected to data-server\n"); fflush(stdout);

			for (uint32_t i = 0; i < MAX_VALUES; i +=2){
				sprintf(linha, "%u", i);
				printf("writing key %d with value %s\n", i, linha); fflush(stdout);
				kv_write(kv, i , linha, strlen(linha)+1, 0);
			}

			printf("\npress enter to read values\n");
			for (uint32_t i = 0; i < MAX_VALUES; i ++){
				if(kv_read(kv, i , linha, 100) == 0){
					printf ("key - %10u value %s\n", i, linha); fflush(stdout);
				}
			}

			kv_close(kv);


		}else{
			int kv = kv_connect("127.0.0.1", 9999);
			printf("connected to data-server\n"); fflush(stdout);

			for (uint32_t i = 1; i < MAX_VALUES; i +=2){
				sprintf(linha, "%u", i);
				printf("writing key %d with value %s\n", i, linha); fflush(stdout);
				kv_write(kv, i , linha, strlen(linha)+1, 0);
			}

			printf("deleting even values\n");
			for (uint32_t i = 0; i < MAX_VALUES; i +=2){		
				printf("deleting key %d\n", i);
				kv_delete(kv, i);
			}

			printf("\nreading values...\n");
			for (uint32_t i = 0; i < MAX_VALUES; i ++){
				if(kv_read(kv, i , linha, 100) == 0){
					printf ("key - %10u value %s\n", i, linha);
				}
			}

			kv_close(kv);
		}
	}
}
