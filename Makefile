all: server.c deliver.c
	gcc -g -Wall -o server server.c
	gcc -g -Wall -o deliver deliver.c

clean:
	$(RM) server
	$(RM) deliver
