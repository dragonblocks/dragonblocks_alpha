COMMON = array.o list.o map.o signal.o util.o types.o node.o queue.o
SERVER = $(COMMON) server.o servercommands.o servermap.o perlin.o facecache.o mapgen.o mapdb.o
CLIENT = $(COMMON) client.o clientcommands.o clientmap.o mesh.o scene.o shaders.o blockmesh.o
LIBRARIES = -lpthread -lm
FLAGS = -g -fmax-errors=4

ifdef RELEASE
FLAGS = -O3 -DRELEASE
endif

all: Dragonblocks DragonblocksServer

Dragonblocks: $(CLIENT)
	cc $(FLAGS) -o Dragonblocks $(CLIENT) $(LIBRARIES) -lGL -lGLEW -lglfw

DragonblocksServer: $(SERVER)
	cc $(FLAGS) -o DragonblocksServer $(SERVER) $(LIBRARIES) -lsqlite3

%.o: %.c
	cc $(FLAGS) -Wall -Wextra -Wpedantic -Werror -isystem ../deps -c -o $@ $<

clean:
	rm -rf *.o

clobber: clean
	rm -rf Dragonblocks DragonblocksServer
