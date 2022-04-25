cmake_minimum_required(VERSION 3.14)
project(Dragonblocks)

# Variables

set(DEPS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../deps")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

if(NOT ASSET_PATH)
	set(ASSET_PATH "../assets/")
endif()

# Dependencies

cmake_policy(SET CMP0077 NEW)

set(SKIP_INSTALL_ALL ON)
set(BUILD_SHARED_LIBS OFF)
set(ZLIB_LIBRARY zlibstatic)

set(FT_DISABLE_ZLIB OFF)
set(FT_DISABLE_BZIP2 ON)
set(FT_DISABLE_PNG ON)
set(FT_DISABLE_HARFBUZZ ON)
set(FT_DISABLE_BROTLI ON)

add_subdirectory("${DEPS_DIR}/glfw" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/glfw")
add_subdirectory("${DEPS_DIR}/glew-cmake" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/glew")
add_subdirectory("${DEPS_DIR}/sqlite3-cmake" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/sqlite3")
add_subdirectory("${DEPS_DIR}/perlin" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/perlin")
add_subdirectory("${DEPS_DIR}/asprintf" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/asprintf")
add_subdirectory("${DEPS_DIR}/dragonnet" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/dragonnet")
add_subdirectory("${DEPS_DIR}/dragonstd" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/dragonstd")
add_subdirectory("${DEPS_DIR}/getline" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/getline")
add_subdirectory("${DEPS_DIR}/stpcpy" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/stpcpy")
add_subdirectory("${DEPS_DIR}/linenoise" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/linenoise")
add_subdirectory("${DEPS_DIR}/zlib" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/zlib")
add_subdirectory("${DEPS_DIR}/freetype" EXCLUDE_FROM_ALL "${CMAKE_CURRENT_BINARY_DIR}/freetype")

# Options

if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	add_compile_definitions("ENABLE_GL_DEBUG")
endif()

add_compile_definitions("USE_DRAGONNET")
add_compile_definitions("GLEW_STATIC")
add_compile_definitions("ASSET_PATH=\"${ASSET_PATH}\"")

add_compile_options(
	-Wall
	-Wextra
	-Werror
	-Wno-address-of-packed-member
	-fmax-errors=4
)

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

add_library(dragonblocks
	"${CMAKE_CURRENT_BINARY_DIR}/types.c"
	common/color.c
	common/config.c
	common/day.c
	common/environment.c
	common/facedir.c
	common/interrupt.c
	common/item.c
	common/node.c
	common/perlin.c
	common/physics.c
	common/terrain.c
)

target_include_directories(dragonblocks
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC ${CMAKE_CURRENT_BINARY_DIR}
	PUBLIC "${DEPS_DIR}/linmath.h"
	PUBLIC "${DEPS_DIR}/endian.h"
	PUBLIC "${DEPS_DIR}/stb"
)

target_link_libraries(dragonblocks
	PUBLIC pthread
	PUBLIC m
	PUBLIC perlin
	PUBLIC dragonnet
	PUBLIC dragonstd
	PUBLIC getline
	PUBLIC zlibstatic
	PUBLIC stpcpy
)

# Client

add_executable(dragonblocks_client
	client/camera.c
	client/client.c
	client/client_auth.c
	client/client_config.c
	client/client_entity.c
	client/client_inventory.c
	client/client_item.c
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
	client/interact.c
	client/light.c
	client/mesh.c
	client/model.c
	client/raycast.c
	client/screenshot.c
	client/shader.c
	client/sky.c
	client/terrain_gfx.c
	client/texture.c
	client/window.c
)

target_link_libraries(dragonblocks_client
	PUBLIC dragonblocks
	PUBLIC libglew_static
	PUBLIC glfw
	PUBLIC asprintf
	PUBLIC linenoise
	PUBLIC freetype
)

# Server

add_executable(dragonblocks_server
	server/biomes.c
	server/database.c
	server/schematic.c
	server/server.c
	server/server_config.c
	server/server_item.c
	server/server_node.c
	server/server_player.c
	server/server_terrain.c
	server/terrain_gen.c
	server/tree.c
	server/tree_physics.c
	server/voxel_depth_search.c
	server/voxel_procedural.c
)

target_link_libraries(dragonblocks_server
	PUBLIC dragonblocks
	PUBLIC sqlite3
)

# Version

add_custom_target(version
	COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${CMAKE_CURRENT_BINARY_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_dependencies(dragonblocks version)

# Types

add_custom_command(
	OUTPUT types.c types.h
	MAIN_DEPENDENCY types.def
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../deps/protogen/protogen.lua"
	COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/mktypes.sh" "${CMAKE_CURRENT_SOURCE_DIR}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

add_custom_target(types
	DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/types.c" "${CMAKE_CURRENT_BINARY_DIR}/types.h"
)

add_dependencies(dragonblocks types)
