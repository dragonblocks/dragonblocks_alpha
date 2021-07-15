COMMON = array.o bintree.o list.o map.o signal.o util.o types.o node.o queue.o

SERVER = $(COMMON) \
	server/facecache.o \
	server/mapdb.o \
	server/mapgen.o \
	server/perlin.o \
	server/server.o \
	server/server_commands.o  \
	server/server_map.o \

CLIENT = $(COMMON) \
	client/blockmesh.o \
	client/camera.o \
	client/client.o \
	client/client_commands.o \
	client/client_map.o \
	client/client_node.o \
	client/client_player.o \
	client/cube.o \
	client/font.o \
	client/game.o \
	client/hud.o \
	client/input.o \
	client/mesh.o \
	client/object.o \
	client/scene.o \
	client/shader.o \
	client/texture.o \
	client/vertex.o \
	client/window.o \

LIBRARIES = -lpthread -lm -lz
FLAGS = -g -fmax-errors=4

ifdef RELEASE
FLAGS = -O3 -DRELEASE
endif

CFLAGS = -Wall -Wextra -Wpedantic -Werror -iquote . -isystem ../deps

all: Dragonblocks DragonblocksServer

Dragonblocks: $(CLIENT)
	cc $(FLAGS) -o Dragonblocks $(CLIENT) $(LIBRARIES) -lGL -lGLEW -lglfw -lfreetype

DragonblocksServer: $(SERVER)
	cc $(FLAGS) -o DragonblocksServer $(SERVER) $(LIBRARIES) -lsqlite3

%.o: %.c
	cc $(FLAGS) $(CFLAGS) -c -o $@ $<

server/%.o: server/%.c
	cc $(FLAGS) $(CFLAGS) -c -o $@ $<

client/%.o: client/%.c
	cc $(FLAGS) $(CFLAGS) -isystem /usr/include/freetype2 -c -o $@ $<

clean:
	rm -rf *.o client/*.o server/*.o

clobber: clean
	rm -rf Dragonblocks DragonblocksServer
