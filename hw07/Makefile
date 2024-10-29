all: client server

client: client.o
	gcc -o client client.o

client.o: client.c
	gcc -c client.c

server: server.o account.o login.o
	gcc -o server server.o account.o login.o

server.o: server.c
	gcc -c server.c

account.o: account.c
	gcc -c account.c

login.o: login.c
	gcc -c login.c

clean:
	rm *.o client server
