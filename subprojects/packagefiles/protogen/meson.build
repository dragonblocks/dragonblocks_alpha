project('protogen')

meson.override_dependency('protogen', declare_dependency())
meson.override_find_program('protogen', configure_file(
	copy: true,
	input: 'protogen.lua',
	output: 'protogen.lua',
))
