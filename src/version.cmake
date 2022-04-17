execute_process(COMMAND git tag --points-at HEAD
	OUTPUT_VARIABLE GIT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

if("${GIT_VERSION}" STREQUAL "")
	execute_process(COMMAND git rev-parse --short HEAD
		OUTPUT_VARIABLE GIT_VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

configure_file(
	version.h.in
	${BINARY_DIR}/version.h
)
