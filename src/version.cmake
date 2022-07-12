execute_process(COMMAND git describe --tags
	OUTPUT_VARIABLE GIT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(
	version.h.in
	${BINARY_DIR}/version.h
)
