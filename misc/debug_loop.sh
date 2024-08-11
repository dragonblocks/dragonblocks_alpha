#!/bin/bash
COMMAND="../misc/debug.sh"

if [[ $1 == "terrain" ]]; then
	COMMAND="../misc/debug_terrain.sh"
fi

while true; do
	if ! $COMMAND; then
		read
	fi
done
