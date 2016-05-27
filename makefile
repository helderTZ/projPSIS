all : clean client-single client-par front-server data-server

data-server : 
	gcc  -o data-server data-server.c database.c -lpthread -std=gnu99 -g -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast

front-server : 
	gcc  -o front-server front-server.c database.c -lpthread -std=gnu99 -g -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast

client-single : 
	gcc  -o client_single cli-exe-1.c psiskv_lib.c -lpthread -std=gnu99 -g 

client-par : 
	gcc  -o client_par cli-exe-par-2.c psiskv_lib.c -lpthread -std=gnu99 -g

clean:
	rm -f *.o *.out data-server front-server client_single client_par

clean-files:
	rm -f *.bin
