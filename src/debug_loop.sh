#!/bin/bash
COMMAND="./debug.sh"

if [[ $1 == "terrain" ]]; then
	COMMAND="./debug_terrain.sh"
fi

while true; do
	if ! $COMMAND; then
		read
	fi
done
