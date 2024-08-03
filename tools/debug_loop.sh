#!/bin/bash
COMMAND="../tools/debug.sh"

if [[ $1 == "terrain" ]]; then
	COMMAND="../tools/debug_terrain.sh"
fi

while true; do
	if ! $COMMAND; then
		read
	fi
done
