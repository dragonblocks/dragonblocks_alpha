#! /bin/bash
COMMAND="./debug.sh"

if [[ $1 == "mapgen" ]]; then
	COMMAND="./debug_mapgen.sh"
fi

while true; do
	if ! $COMMAND; then
		read
	fi
done
