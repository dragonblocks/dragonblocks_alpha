COMMON = array.o binsearch.o linkedlist.o map.o signal.o util.o types.o
SERVER = $(COMMON) server.o servercommands.o
CLIENT = $(COMMON) client.o clientcommands.o

all: Dragonblocks DragonblocksServer

Dragonblocks: $(CLIENT)
	cc -g -o Dragonblocks $(CLIENT) -pthread -lm

DragonblocksServer: $(SERVER)
	cc -g -o DragonblocksServer $(SERVER) -pthread -lm

%.o: %.c
	cc -c -g -o $@ -Wall -Wextra -Wpedantic -Werror $<

clean:
	rm -rf *.o

clobber: clean
	rm -rf Dragonblocks DragonblocksServer
