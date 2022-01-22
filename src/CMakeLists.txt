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

include_directories(SYSTEM
	${DEPS_DIR}
)

include_directories(BEFORE
	${CMAKE_SOURCE_DIR}
)

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "FreeBSD" OR "${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
	link_directories("/usr/local/lib")
	include_directories("/usr/local/include")
endif()

if ("${CMAKE_SYSTEM_NAME}" STREQUAL "OpenBSD")
	link_directories("/usr/X11R6/lib")
	include_directories("/usr/X11R6/include")
endif()

add_compile_options(-Wall -Wextra -Werror)

set(DEPS_SOURCES
	"${DEPS_DIR}/dragontype/array.c"
	"${DEPS_DIR}/dragontype/bintree.c"
	"${DEPS_DIR}/dragontype/list.c"
	"${DEPS_DIR}/dragontype/number.c"
	"${DEPS_DIR}/dragontype/queue.c"
	"${DEPS_DIR}/dragonport/asprintf.c"
	"${DEPS_DIR}/perlin/perlin.c"
)

set(COMMON_SOURCES
	${DEPS_SOURCES}
	config.c
	day.c
	environment.c
	map.c
	node.c
	perlin.c
	signal_handlers.c
	util.c
)

add_executable(Dragonblocks
	${COMMON_SOURCES}
	client/blockmesh.c
	client/camera.c
	client/client.c
	client/client_commands.c
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

add_executable(DragonblocksServer
	${COMMON_SOURCES}
	server/biomes.c
	server/database.c
	server/mapgen.c
	server/server.c
	server/server_commands.c
	server/server_config.c
	server/server_map.c
	server/trees.c
	server/voxelctx.c
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
