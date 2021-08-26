execute_process(COMMAND git tag --points-at HEAD
	OUTPUT_VARIABLE GIT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

if ("${GIT_VERSION}" STREQUAL "")
	execute_process(COMMAND git rev-parse --short HEAD
		OUTPUT_VARIABLE GIT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

set(VERSION_DEF "const char *version = \"${GIT_VERSION}\";")

configure_file(
	${CMAKE_SOURCE_DIR}/version.h.in
	${CMAKE_SOURCE_DIR}/version.h
)
