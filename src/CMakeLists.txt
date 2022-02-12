cmake_minimum_required(VERSION 3.12)
project(Dragonblocks)

# Variables

set(DEPS_DIR "${CMAKE_SOURCE_DIR}/../deps/")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

if(NOT RESSOURCE_PATH)
	set(RESSOURCE_PATH "../")
endif()

# Dependencies

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(Freetype REQUIRED)

# Options

add_compile_definitions("RESSOURCE_PATH=\"${RESSOURCE_PATH}\"")
add_compile_definitions("USE_DRAGONNET")

add_compile_options(-Wall -Wextra -Werror -fmax-errors=4)

link_libraries(
	pthread
	m
	z
)

include_directories(SYSTEM ${DEPS_DIR})
include_directories(BEFORE ${CMAKE_SOURCE_DIR})

# System specific options

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD" OR "${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
	link_directories("/usr/local/lib")
	include_directories("/usr/local/include")
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
	link_directories("/usr/X11R6/lib")
	include_directories("/usr/X11R6/include")
endif()

# Common sources

set(COMMON_SOURCES
	"${DEPS_DIR}/dragonnet/addr.c"
	"${DEPS_DIR}/dragonnet/listen.c"
	"${DEPS_DIR}/dragonnet/peer.c"
	"${DEPS_DIR}/dragonnet/recv.c"
	"${DEPS_DIR}/dragonnet/recv_thread.c"
	"${DEPS_DIR}/dragonnet/send.c"
	"${DEPS_DIR}/dragonport/asprintf.c"
	"${DEPS_DIR}/dragonstd/array.c"
	"${DEPS_DIR}/dragonstd/bintree.c"
	"${DEPS_DIR}/dragonstd/list.c"
	"${DEPS_DIR}/dragonstd/queue.c"
	"${DEPS_DIR}/linenoise/linenoise.c"
	"${DEPS_DIR}/perlin/perlin.c"
	config.c
	day.c
	environment.c
	map.c
	node.c
	perlin.c
	signal_handlers.c
	types.c
	util.c
)

# Client

add_executable(Dragonblocks
	${COMMON_SOURCES}
	client/blockmesh.c
	client/camera.c
	client/client.c
	client/client_auth.c
	client/client_config.c
	client/client_map.c
	client/client_node.c
	client/client_player.c
	client/cube.c
	client/debug_menu.c
	client/facecache.c
	client/font.c
	client/frustum.c
	client/game.c
	client/gui.c
	client/input.c
	client/mesh.c
	client/object.c
	client/scene.c
	client/shader.c
	client/sky.c
	client/texture.c
	client/vertex.c
	client/window.c
)

target_link_libraries(Dragonblocks
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	glfw
	${FREETYPE_LIBRARIES}
)

target_include_directories(Dragonblocks PUBLIC
	${FREETYPE_INCLUDE_DIRS}
)

# Server

add_executable(DragonblocksServer
	${COMMON_SOURCES}
	server/biomes.c
	server/database.c
	server/mapgen.c
	server/server.c
	server/server_config.c
	server/server_map.c
	server/server_player.c
	server/trees.c
	server/voxelctx.c
)

target_link_libraries(DragonblocksServer
	sqlite3
)

# Version

add_custom_target(Version
	COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/version.cmake
)

add_dependencies(Dragonblocks Version)
add_dependencies(DragonblocksServer Version)

# Types

add_custom_command(
	OUTPUT "${CMAKE_SOURCE_DIR}/types.c" "${CMAKE_SOURCE_DIR}/types.h"
	COMMAND "${CMAKE_SOURCE_DIR}/mktypes.sh"
	MAIN_DEPENDENCY "${CMAKE_SOURCE_DIR}/types.def"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_custom_target(DragonnetTypes
	DEPENDS "${CMAKE_SOURCE_DIR}/types.c" "${CMAKE_SOURCE_DIR}/types.h"
)

add_dependencies(Dragonblocks DragonnetTypes)
add_dependencies(DragonblocksServer DragonnetTypes)
