#include "database.h"

#define DbInitSize 10

dictionary * database;

int dictionary_init(){
	int i;
	//dictionary * dbaux;
	//verificar se existe backup file, se existir fazer backup

	//se houver backup alocar o tamanho necessario ao backup
	database = (dictionary *) malloc(sizeof(dictionary));
	if (database==NULL)
		return -1;
	
	database->next=database;
	database->prev=database;
	/*dbaux=database;
	printf("antes do for\n");
	for (i = 0; i < DbInitSize; i++){
		dbaux[i].value=NULL;
		dbaux[i].value_length=0;
		if(i<DbInitSize-1){
			dbaux[i].next=&dbaux[i+1];
			dbaux[i+1].prev=&dbaux[i];
		}else
			dbaux[i].next=dbaux;
	}
	database->prev=&dbaux[DbInitSize-1];
	printf("depois do for\n");
	*/
	return 0;

}

dictionary * find_entry(uint32_t key){
	dictionary *aux = database;
	int i=0;
	//Critical region
	if(aux->key==key) return aux;

	while(aux->next != database){
		//printf("it=%d\n", i++);
		aux = aux->next;
		if(aux->key==key)
			return aux;
	}
	return NULL;
}

/** add_entry function*************************
@return 1 if entry already exists
@return 0 if no errors
@return -1 if errors found
************************************************/
int add_entry(uint32_t key, void * value, uint32_t value_length, int overwrite ){
	dictionary *new_entry;
	dictionary entry_found, * entry_found_ptr;

	entry_found_ptr=find_entry(key);
	if(entry_found_ptr!=NULL){//if entry exists

		entry_found=*entry_found_ptr;	
		
		if (overwrite){
			value = (void *) malloc(value_length);
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
			return 1;

	}else{//add new entry
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

void main(void){
	dictionary *entry;
	int value=150;
	uint32_t key=10;

	if(dictionary_init()==-1)
		printf("init error\n");
	
	if(add_entry(key,(void *)&value,sizeof(int),0)==-1)
		printf("add entry error\n");

	entry = find_entry(10);
	if(entry==NULL)
		printf("entry null\n");
	else
		printf("key=%d value=%d value_length=%d\n", entry->key, *((int *)entry->value), entry->value_length );

	if(add_entry(50,(void *)&value,sizeof(int),0)==-1)
		printf("add entry error\n");

	entry = find_entry(10);
	if(entry==NULL)
		printf("entry null\n");
	else
		printf("key=%d value=%d value_length=%d\n", entry->key, *((int *)entry->value), entry->value_length );

}