project('dragonblocks', 'c', default_options: ['warning_level=2'])

add_project_arguments([
	'-Wno-address-of-packed-member',
	'-DGLEW_NO_GLU',
	'-DUSE_DRAGONNET',
	'-DASSET_PATH="' + get_option('asset_path') + '"',
], language: 'c')

cc = meson.get_compiler('c')

include = 'src/'

deps = [
	cc.find_library('m', required: false),
	dependency('threads'),
	dependency('protogen'),
	dependency('endian'),
	dependency('linmath'),
	dependency('perlin'),
	dependency('dragonnet'),
	dependency('dragonstd'),
	dependency('getline'),
	dependency('zlib'),
]

common_lib = static_library('dragonblocks',
	sources: [
		vcs_tag(
			command: [find_program('git'), 'describe', '--tags'],
			input: 'src/version.h.in',
			output: 'version.h',
			replace_string: '@GIT_VERSION@',
		),
		custom_target('protogen_types',
			output: ['types.c', 'types.h'],
			input: 'src/types.def',
			command: [find_program('protogen'), '@INPUT@', '@OUTPUT@'],
		),
		'src/common/color.c',
		'src/common/config.c',
		'src/common/day.c',
		'src/common/environment.c',
		'src/common/facedir.c',
		'src/common/init.c',
		'src/common/interrupt.c',
		'src/common/item.c',
		'src/common/node.c',
		'src/common/perlin.c',
		'src/common/physics.c',
		'src/common/terrain.c',
	],
	dependencies: deps,
	include_directories: include,
)

common = declare_dependency(
	link_with: common_lib,
	dependencies: deps,
	include_directories: include,
)

executable('dragonblocks-client',
	sources: [
		'src/client/camera.c',
		'src/client/client.c',
		'src/client/client_auth.c',
		'src/client/client_config.c',
		'src/client/client_entity.c',
		'src/client/client_inventory.c',
		'src/client/client_item.c',
		'src/client/client_node.c',
		'src/client/client_player.c',
		'src/client/client_terrain.c',
		'src/client/cube.c',
		'src/client/debug_menu.c',
		'src/client/facecache.c',
		'src/client/font.c',
		'src/client/frustum.c',
		'src/client/game.c',
		'src/client/gui.c',
		'src/client/input.c',
		'src/client/interact.c',
		'src/client/light.c',
		'src/client/mesh.c',
		'src/client/model.c',
		'src/client/opengl.c',
		'src/client/raycast.c',
		'src/client/screenshot.c',
		'src/client/shader.c',
		'src/client/sky.c',
		'src/client/terrain_gfx.c',
		'src/client/texture.c',
		'src/client/window.c',
	],
	dependencies: [
		common,
		dependency('stb'),
		dependency('glew'),
		dependency('gl'),
		dependency('glfw3'),
		dependency('asprintf'),
		dependency('linenoise'),
		dependency('freetype2'),
	],
	install: true,
)

executable('dragonblocks-server',
	sources: [
		'src/server/biomes.c',
		'src/server/database.c',
		'src/server/schematic.c',
		'src/server/server.c',
		'src/server/server_config.c',
		'src/server/server_item.c',
		'src/server/server_node.c',
		'src/server/server_player.c',
		'src/server/server_terrain.c',
		'src/server/terrain_gen.c',
		'src/server/tree.c',
		'src/server/tree_physics.c',
		'src/server/voxel_depth_search.c',
		'src/server/voxel_procedural.c',
	],
	dependencies: [
		common,
		dependency('sqlite3'),
	],
	install: true,
)
