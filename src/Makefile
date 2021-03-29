COMMON = array.o list.o map.o signal.o util.o types.o node.o
SERVER = $(COMMON) server.o servercommands.o mapgen.o perlin.o
CLIENT = $(COMMON) client.o clientcommands.o mesh.o scene.o mapblock_meshgen.o shaders.o
LIBRARIES = -lpthread -lm
CLIENT_LIBRARIES = -lGL -lGLEW -lglfw
FLAGS = -g -fmax-errors=4

ifdef RELEASE
FLAGS = -O3 -DRELEASE
endif

all: Dragonblocks DragonblocksServer

Dragonblocks: $(CLIENT)
	cc $(FLAGS) -o Dragonblocks $(CLIENT) $(LIBRARIES) $(CLIENT_LIBRARIES)

DragonblocksServer: $(SERVER)
	cc $(FLAGS) -o DragonblocksServer $(SERVER) $(LIBRARIES)

%.o: %.c
	cc $(FLAGS) -o $@ -c -Wall -Wextra -Wpedantic -Werror -isystem ../deps $<

clean:
	rm -rf *.o

clobber: clean
	rm -rf Dragonblocks DragonblocksServer
