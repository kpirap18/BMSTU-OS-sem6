all: client server

client: socket.h client.c
	gcc -g client.c -o client

server: socket.h server.c
	gcc -g server.c -o server

clean:
	rm -rf *.o
	rm -rf *.soc