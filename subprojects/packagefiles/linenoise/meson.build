project('linenoise', 'c')

linenoise = library('linenoise', sources: ['utf8.c', 'stringbuf.c', 'linenoise.c'])

meson.override_dependency('linenoise', declare_dependency(link_with: linenoise, include_directories: '.'))
