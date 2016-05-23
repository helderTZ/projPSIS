#include "database.h"

#define DbInitSize 10

dictionary * database;
char isEmpty=1;

int dictionary_init(){
	int i;
	//verificar se existe backup file, se existir fazer backup

	//se houver backup alocar o tamanho necessario ao backup
	database = (dictionary *) malloc(sizeof(dictionary));
	if (database==NULL)
		return -1;
	
	database->next=database;
	database->prev=database;
	isEmpty=1;
	return 0;

}

dictionary * find_entry(uint32_t key){
	dictionary *aux = database;
	int i=0;

	if(isEmpty) return NULL;

	//Critical region
	if(aux->key==key) return aux;

	while(aux->next != database){
		aux = aux->next;
		if(aux->key==key)
			return aux;
	}
	return NULL;
}

/** add_entry function*************************
@return -2 if entry already exists and not overwrite
@return 0 if no errors
@return -1 if errors found
************************************************/
int add_entry(uint32_t key, void * value, uint32_t value_length, int overwrite ){
	dictionary *new_entry;
	dictionary entry_found, * entry_found_ptr;

	//DEBUG
	static int blahblah=0;

	if(isEmpty) {
		database->key = key;
		database->value = value;
		database->value_length = value_length;
		isEmpty = 0;
	}

	entry_found_ptr=find_entry(key);
	if(entry_found_ptr!=NULL){//if entry exists

		entry_found=*entry_found_ptr;	
		
		if (overwrite){
			//value = (void *) malloc(value_length);
			printf("Inside overwrite %d\n", blahblah++); fflush(stdout);
			new_entry =(dictionary *) malloc(sizeof(dictionary));
		
			new_entry->prev = entry_found_ptr->prev;//copy old entry into new entry prev pointer
			new_entry->next = entry_found_ptr->next;//copy old entry into new entry next pointer
			new_entry->value_length=value_length;//refresh value lenght
			new_entry->value=value;	//refresh value
			new_entry->key=key;
			free(entry_found_ptr->value);
			free(entry_found_ptr);
			
			return 0;
		}else//if already exists and not overwrite -> do nothing
			return -2;

	}else{//add new entry
		isEmpty=0;

		new_entry =(dictionary *) malloc(sizeof(dictionary));
		new_entry->value = (void *) malloc(value_length);
		
		new_entry->prev=database->prev;
		new_entry->next=database;//new entry next point to first entry
		database->prev->next=new_entry;//new entry point to the last entry
		database->prev=new_entry;//1st entry prev point to last entry
		new_entry->key=key;
		memcpy(new_entry->value,value,value_length);
		new_entry->value_length=value_length;
		return 0;
	}
		return -1;
}

/*
@return 1 if entry not exists
@return 0 if entry sucessfuly removed
@return -1 if error ocurred
*/
int delete_entry(uint32_t key){
	dictionary *aux;
	aux=find_entry(key);
	if (aux!=NULL){
		aux->prev->next=aux->next;
		aux->next->prev=aux->prev;
		free(aux->value);
		free(aux);
		return 0;
	}else return -1;
}

/* 
Warning: Do not forget to free the memory after read_entry function
@param entry points to found entry in order to be read
@return 1 if entry not exists
@return 0 if entry sucessfuly read
@return -1 if error ocurred
*/
int read_entry(uint32_t key, dictionary ** entry){
	dictionary *aux;
	aux = find_entry(key);
	if(aux!=NULL){
		*entry = (dictionary*) malloc(sizeof(dictionary));
		memcpy(*entry, aux, sizeof(dictionary));
		return 0;
	}else return -2;
	
	return -1;

}

void printList() {

	dictionary* aux = database;
	while(aux->next!=database) {
		aux = aux->next;
		printf("key = %d\tvalue = %s\n", aux->key, (char*)aux->value);
		
	}

}

/*
void main(void){
	dictionary *entry;
	int * value;
	uint32_t key=10;
	dictionary * read_value;

	value = (int *) malloc(sizeof(int));
	*value =150;

	if(dictionary_init()==-1)
		printf("init error\n");

	printf("after init\n");

	if(add_entry(key, (void *) value,sizeof(int),0)==-1)
		printf("add entry error\n");

	printf("after add\n");

	entry = find_entry(10);
	if(entry==NULL)
		printf("entry null\n");
	else
		printf("key=%d value=%d value_length=%d\n", entry->key, *((int *)entry->value), entry->value_length );

	if(add_entry(50,(void *)&value,sizeof(int),0)==-1)
		printf("add entry error\n");

	entry = find_entry(50);
	if(entry==NULL)
		printf("entry null\n");
	else
		printf("key=%d value=%d value_length=%d\n", entry->key, *((int *)entry->value), entry->value_length );

	
	if(delete_entry(key))
		printf("Remove: value not exists\n");
	else
		printf("value removed\n");
	entry = find_entry(10);
	if (entry!=NULL)
		printf("key=%d value=%d value_length=%d\n", entry->key, *((int *)entry->value), entry->value_length );
	else
		printf("Not exists\n");

	if(read_entry(50, &read_value)==0)
		printf("read_value=%d\n", (*(int *)read_value));
	else
		printf("read_value:error\n");
}*/