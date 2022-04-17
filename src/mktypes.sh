#!/bin/bash
SOURCE_DIR="$1"

if [[ "$SOURCE_DIR" = "" ]]; then
	SOURCE_DIR="."
fi

export LUA_PATH="$SOURCE_DIR/../deps/protogen/?.lua;$SOURCE_DIR/../deps/protogen/?/init.lua"
"$SOURCE_DIR/../deps/protogen/protogen.lua" "$SOURCE_DIR/types.def"
