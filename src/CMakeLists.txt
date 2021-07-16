cmake_minimum_required(VERSION 3.12)

project(Dragonblocks)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

link_libraries(
	pthread
	m
	z
)

include_directories(SYSTEM
	"${CMAKE_SOURCE_DIR}/../deps/"
)

include_directories(BEFORE
	${CMAKE_SOURCE_DIR}
)

if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

set(SOURCES_COMMON
	array.c
	bintree.c
	list.c
	map.c
	signal_handlers.c
	util.c
	types.c
	node.c
	queue.c
)

add_executable(Dragonblocks
	${SOURCES_COMMON}
	client/blockmesh.c
	client/camera.c
	client/client.c
	client/client_commands.c
	client/client_map.c
	client/client_node.c
	client/client_player.c
	client/cube.c
	client/font.c
	client/game.c
	client/hud.c
	client/input.c
	client/mesh.c
	client/object.c
	client/scene.c
	client/shader.c
	client/texture.c
	client/vertex.c
	client/window.c
)

target_link_libraries(Dragonblocks
	GL
	GLEW
	glfw
	freetype
)

target_include_directories(Dragonblocks PUBLIC
	/usr/include/freetype2/
)

add_executable(DragonblocksServer
	${SOURCES_COMMON}
	server/facecache.c
	server/mapdb.c
	server/mapgen.c
	server/perlin.c
	server/server.c
	server/server_commands.c
	server/server_map.c
)

target_link_libraries(DragonblocksServer
	sqlite3
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compile_definitions(RELEASE)
endif()
