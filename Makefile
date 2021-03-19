COMMON = array.o binsearch.o linkedlist.o map.o util.o
SERVER = $(COMMON) server.o
CLIENT = $(COMMON) client.o

all: Dragonblocks DragonblocksServer

Dragonblocks: $(CLIENT)
	gcc -g -o Dragonblocks $(CLIENT)

DragonblocksServer: $(SERVER)
	gcc -g -o DragonblocksServer $(SERVER)

%.o: %.c
	gcc -c -g -o $@ -Wall -Wextra -Wpedantic -Werror $<

clean:
	rm *.o
