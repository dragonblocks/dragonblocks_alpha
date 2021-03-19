COMMON = array.o binsearch.o linkedlist.o map.o
SERVER = $(COMMON) server.o

all: DragonblocksServer

DragonblocksServer: $(SERVER)
	gcc -g -o DragonblocksServer $(SERVER)

%.o: %.c
	gcc -c -g -o $@ -Wall -Wextra -Wpedantic $<

clean:
	rm *.o
