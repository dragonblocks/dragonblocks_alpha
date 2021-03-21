COMMON = array.o binsearch.o linkedlist.o map.o util.o types.o
SERVER = $(COMMON) server.o server_command_handlers.o
CLIENT = $(COMMON) client.o

all: Dragonblocks DragonblocksServer

Dragonblocks: $(CLIENT)
	cc -g -o Dragonblocks $(CLIENT)

DragonblocksServer: $(SERVER)
	cc -g -o DragonblocksServer $(SERVER) -pthread

%.o: %.c
	cc -c -g -o $@ -Wall -Wextra -Wpedantic -Werror $<

clean:
	rm -rf *.o

clobber: clean
	rm -rf Dragonblocks DragonblocksServer
