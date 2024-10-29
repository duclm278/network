all: client server

client: client.o
	gcc -o client client.o

client.o: client.c
	gcc -c client.c

server: server.o account.o
	gcc -o server server.o account.o

server.o: server.c
	gcc -c server.c

account.o: account.c
	gcc -c account.c

clean:
	rm *.o client server
