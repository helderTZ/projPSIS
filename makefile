all : clean client-single client-par-1 client-par-2 front-server data-server

data-server : 
	gcc  -o data-server data-server.c database.c -lpthread -std=gnu99 -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -g

front-server : 
	gcc  -o front-server front-server.c database.c -lpthread -std=gnu99 -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -g

client-single : 
	gcc  -o client_single cli-exe-1.c psiskv_lib.c -lpthread -std=gnu99 -g

client-par-1 : 
	gcc  -o client_par-1 cli-exe-par-1.c psiskv_lib.c -lpthread -std=gnu99 -g
	
client-par-2 : 
	gcc  -o client_par-2 cli-exe-par-2.c psiskv_lib.c -lpthread -std=gnu99 -g

clean:
	rm -f *.o *.out data-server front-server client_single client_par

clean-files:
	rm -f *.bin
