#include "psiskv.h"

#include <stdio.h>
#include <string.h>

#define MAX_VALUES 10

int main(){

	char linha[100];
	int kv = kv_connect("127.0.0.1", 9999);
	if(kv==-1){
		perror("kv_connect");
		exit(-1);
	}

	printf("\nwriting first values...\n");
	for (uint32_t i = 0; i < MAX_VALUES; i ++){
		sprintf(linha, "%u", i);
		printf("writing key %d with value %s\n", i, linha);
		kv_write(kv, i , linha, strlen(linha)+1, 0);
	}

	printf("\npress enter to read values\n");
	getchar();
	for (uint32_t i = 0; i < MAX_VALUES; i ++){
		if(kv_read(kv, i , linha, 100) == 0){
			printf ("key - %10u value %s\n", i, linha);
		}
	}

	printf("press enter to delete even values\n");
	getchar();
	for (uint32_t i = 0; i < MAX_VALUES; i +=2){		
		printf("deleting key %d\n", i);
		kv_delete(kv, i);
	}

	printf("press enter to read values\n");
	getchar();
	for (uint32_t i = 0; i < MAX_VALUES; i ++){
		if(kv_read(kv, i , linha, 100) == 0){
			printf ("key - %10u value %s\n", i, linha);
		}else {
			printf("key - %10u does not exist in dictionary\n", i);
		}
	}

	printf("\nwriting new values...\n");
	for (uint32_t i = 0; i < MAX_VALUES; i ++){
		sprintf(linha, "%u", i*10);
		printf("writing key %d with value %s\n", i, linha);
		kv_write(kv, i , linha, strlen(linha)+1, 0); /* will not overwrite*/
	}

	printf("press enter to read new values\n");
	getchar();
	for (uint32_t i = 0; i < MAX_VALUES; i ++){
		if(kv_read(kv, i , linha, 100) == 0){
			printf ("key - %10u value %s\n", i, linha);
		}
	}
	kv_close(kv);
	
}
