cmake_minimum_required(VERSION 3.12)

project(Dragonblocks)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(Freetype REQUIRED)

link_libraries(
	pthread
	m
	z
)

set(DEPS_DIR "${CMAKE_SOURCE_DIR}/../deps/")

add_compile_definitions(DRAGONTYPE_ENDIAN_HEADER="${CMAKE_SOURCE_DIR}/../deps/endian.h/endian.h")

include_directories(SYSTEM
	${DEPS_DIR}
)

include_directories(BEFORE
	${CMAKE_SOURCE_DIR}
)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

set(SOURCES_COMMON
	environment.c
	map.c
	node.c
	perlin.c
	signal_handlers.c
	util.c
	"${DEPS_DIR}/dragontype/implementation.c"
	"${DEPS_DIR}/perlin/perlin.c"
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
	client/debug_menu.c
	client/facecache.c
	client/font.c
	client/game.c
	client/gui.c
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
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES}
	glfw
	${FREETYPE_LIBRARIES}
)

target_include_directories(Dragonblocks PUBLIC
	${FREETYPE_INCLUDE_DIRS}
)

add_executable(DragonblocksServer
	${SOURCES_COMMON}
	server/biomes.c
	server/database.c
	server/mapgen.c
	server/server.c
	server/server_commands.c
	server/server_map.c
)

target_link_libraries(DragonblocksServer
	sqlite3
)

add_custom_target(version
	COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/version.cmake
)

add_dependencies(Dragonblocks version)
add_dependencies(DragonblocksServer version)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
	add_compile_definitions(RELEASE)
endif()
