all:
	gcc -o frontend frontend.c -lpthread
	gcc -o backend backend.c -lpthread users_lib.o
	rm serverPipe
	rm heartbeatPipe
	rm CLIENTE*


frontend:
	gcc -c frontend.c -lpthread


background:
	gcc -c backend.c -lpthread


clean:
	$rm backend frontend backend.o frontend.o serverpipe USER_FIFO*
