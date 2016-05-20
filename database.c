#include "database.h"

#define SIZEOFDB 10

dictionary * database;

int dictionary_init(){

	//verificar se existe backup file, se existir fazer backup

	//se houver backup alocar o tamanho necessario ao backup
	database = (dictionary *) malloc(SIZEOFDB * sizeof(dictionary));
	if (database==NULL)
		return -1;
	
	database->next=NULL;
	database->prev=NULL;
	
	return 0;

}

dictionary find_entry(uint32_t key, int find_last){
	dictionary aux;
	aux=*database;

	//Critical region
	while(aux.next !=NULL){
		if(aux.key==key)
			return aux;
		&aux = aux.next;
	}
	return NULL;
}

int add_entry(uint32_t key, void * value, uint32_t value_lenght, int overwrite ){
	dictionary *new_entry;
	dictionary entry_found;
	void * value;

	entry_found=find_node(key);

	if(entry_found!=NULL){
		value = (void *) malloc(value_lenght);
		new_entry =(dictionary *) malloc(sizeof(dictionary));
		
		if (overwrite){
			*new_entry = entry_found;//copy old entry into new entry
					
			free(entry_found->value);
			free(entry_found);
		}else{//add new entry
			database->prev->next=new_entry;
			new_entry->next=database;
			new_entry->prev=database->prev->prev;
		}

		new_entry->value_lenght=value_lenght;//refresh value lenght
		new_entry->value=value;	//refresh value
		return 0;
	}else
		return -1;
}

void main(void){


}