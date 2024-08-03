project('perlin', 'c', default_options : ['warning_level=3'])

cc = meson.get_compiler('c')
perlin = library('perlin', 'perlin.c',
  dependencies: cc.find_library('m'), install: true)

meson.override_dependency('perlin', declare_dependency(link_with: perlin, include_directories: '.'))
