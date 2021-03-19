COMMON = array.o binsearch.o linkedlist.o map.o
SERVER = $(COMMON) server.o

all: DragonblocksServer

DragonblocksServer: $(SERVER)
	gcc -o DragonblocksServer $(SERVER)

%.o: %.c
	gcc -c -o $@ -Wall -Wextra -Wpedantic $<
