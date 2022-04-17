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

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	add_compile_definitions("ENABLE_GL_DEBUG")
endif()

add_compile_definitions("USE_DRAGONNET")
add_compile_definitions("RESSOURCE_PATH=\"${RESSOURCE_PATH}\"")

add_compile_options(-Wall -Wextra -Werror -Wno-address-of-packed-member -fmax-errors=4)

link_libraries(
	pthread
	m
	z
)

include_directories(SYSTEM ${DEPS_DIR})
include_directories(BEFORE ${CMAKE_SOURCE_DIR})

# System specific options

if("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD" OR "${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
	link_directories("/usr/local/lib")
	include_directories("/usr/local/include")
endif()

if("${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
	link_directories("/usr/X11R6/lib")
	include_directories("/usr/X11R6/include")
endif()

# Common sources

set(COMMON_SOURCES
	"${DEPS_DIR}/asprintf/asprintf.c"
	"${DEPS_DIR}/dragonnet/addr.c"
	"${DEPS_DIR}/dragonnet/listen.c"
	"${DEPS_DIR}/dragonnet/peer.c"
	"${DEPS_DIR}/dragonnet/recv.c"
	"${DEPS_DIR}/dragonnet/recv_thread.c"
	"${DEPS_DIR}/dragonnet/send.c"
	"${DEPS_DIR}/dragonstd/array.c"
	"${DEPS_DIR}/dragonstd/flag.c"
	"${DEPS_DIR}/dragonstd/list.c"
	"${DEPS_DIR}/dragonstd/map.c"
	"${DEPS_DIR}/dragonstd/queue.c"
	"${DEPS_DIR}/dragonstd/refcount.c"
	"${DEPS_DIR}/dragonstd/tree.c"
	"${DEPS_DIR}/dragonstd/bits/compare.c"
	"${DEPS_DIR}/linenoise/linenoise.c"
	"${DEPS_DIR}/perlin/perlin.c"
	color.c
	config.c
	day.c
	environment.c
	interrupt.c
	node.c
	perlin.c
	physics.c
	terrain.c
	types.c
)

# Client

add_executable(dragonblocks
	${COMMON_SOURCES}
	client/camera.c
	client/client.c
	client/client_auth.c
	client/client_config.c
	client/client_entity.c
	client/client_node.c
	client/client_player.c
	client/client_terrain.c
	client/cube.c
	client/debug_menu.c
	client/facecache.c
	client/font.c
	client/frustum.c
	client/game.c
	client/gl_debug.c
	client/gui.c
	client/input.c
	client/light.c
	client/mesh.c
	client/model.c
	client/shader.c
	client/sky.c
	client/terrain_gfx.c
	client/texture.c
	client/window.c
)

target_link_libraries(dragonblocks
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	glfw
	${FREETYPE_LIBRARIES}
)

target_include_directories(dragonblocks PUBLIC
	${FREETYPE_INCLUDE_DIRS}
)

# Server

add_executable(dragonblocks_server
	${COMMON_SOURCES}
	server/biomes.c
	server/database.c
	server/schematic.c
	server/server.c
	server/server_config.c
	server/server_player.c
	server/server_terrain.c
	server/terrain_gen.c
	server/trees.c
	server/voxelctx.c
)

target_link_libraries(dragonblocks_server
	sqlite3
)

# Version

add_custom_target(version
	COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/version.cmake
)

add_dependencies(dragonblocks version)
add_dependencies(dragonblocks_server version)

# Types

add_custom_command(
	OUTPUT "${CMAKE_SOURCE_DIR}/types.c" "${CMAKE_SOURCE_DIR}/types.h"
	COMMAND "${CMAKE_SOURCE_DIR}/mktypes.sh"
	MAIN_DEPENDENCY "${CMAKE_SOURCE_DIR}/types.def"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_custom_target(types
	DEPENDS "${CMAKE_SOURCE_DIR}/types.c" "${CMAKE_SOURCE_DIR}/types.h"
)

add_dependencies(dragonblocks types)
add_dependencies(dragonblocks_server types)
