all : clean data-server client-single

data-server : 
	gcc  -o data-server data-server.c database.c -lpthread -g

front-server : 
	gcc  -o front-server front-server.c -lpthread -g 

client-single : 
	gcc  -o client_single cli-exe-1.c psiskv_lib.c -lpthread -std=gnu99 -g

client-par : 
	gcc  -o client_par cli-exe-par-1.c psiskv_lib.c -lpthread -std=gnu99 -g

clean:
	rm -f *.o *.out data-server front-server client_single client_par


