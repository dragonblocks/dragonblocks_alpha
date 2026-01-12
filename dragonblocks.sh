#!/bin/sh
# dragonblocks launcher script
# shellcheck disable=SC3043
set -e

script_name="$0"

if [ "$(uname)" = "Darwin" ]; then
	: "${DRAGONBLOCKS_DATA:=${HOME}/Library/Application Support/dragonblocks_alpha/data}"
	: "${DRAGONBLOCKS_CONFIG:=${HOME}/Library/Application Support/dragonblocks_alpha/config}"
	: "${DRAGONBLOCKS_TMP:=${TMPDIR:-/tmp}/dragonblocks_alpha}"
	: "${DRAGONBLOCKS_STATE:=${HOME}/Library/Application Support/dragonblocks_alpha/state}"
else
	: "${DRAGONBLOCKS_DATA:=${XDG_DATA_HOME:-$HOME/.local/share}/dragonblocks_alpha}"
	: "${DRAGONBLOCKS_CONFIG:=${XDG_CONFIG_HOME:-$HOME/.config}/dragonblocks_alpha}"
	: "${DRAGONBLOCKS_TMP:=${XDG_RUNTIME_DIR:-/tmp}/dragonblocks_alpha}"
	: "${DRAGONBLOCKS_STATE:=${XDG_STATE_HOME:-$HOME/.local/state}/dragonblocks_alpha}"
fi

mkdir -p "$DRAGONBLOCKS_DATA/"
mkdir -p "$DRAGONBLOCKS_CONFIG/"
mkdir -p "$DRAGONBLOCKS_TMP/"
mkdir -p "$DRAGONBLOCKS_STATE/"

version_path="$DRAGONBLOCKS_DATA/versions"
world_path="$DRAGONBLOCKS_STATE/worlds"
log_path="$DRAGONBLOCKS_STATE/logs"
screenshot_path="$DRAGONBLOCKS_STATE/screenshots"
default_version_path="$DRAGONBLOCKS_CONFIG/default_version"

mkdir -p "$version_path/"
mkdir -p "$world_path/"
mkdir -p "$log_path/"
mkdir -p "$screenshot_path/"

: "${DRAGONBLOCKS_URL:=https://dragonblocks.lizzy.rs}"

load_version() {
	case "$1" in
		system)
			version_exec_dir=""
			version_working_dir="."
			;;

		/*)
			version_exec_dir="./"
			version_working_dir="$1"
			;;

		*)
			version_exec_dir="./"
			version_working_dir="$version_path/$1"
			;;
	esac
}

timestamp() {
	date +%Y-%m-%d-%H:%M:%S
}

local_version() {
	[ -x "./dragonblocks-client" ] && echo "$PWD"
}

system_version() {
	command -v "dragonblocks-client" >/dev/null 2>&1 && echo "system"
}

default_version() {
	if [ -n "$DRAGONBLOCKS_VERSION" ]; then
		echo "$DRAGONBLOCKS_VERSION"
	elif [ -f "$default_version_path" ]; then
		cat "$default_version_path"
	elif ! local_version && ! system_version; then
		>&2 echo "no dragonblocks installation found. use '$script_name version download' to obtain versions"
		return 1
	fi
}

get_world() {
	local world path
	world="${1:?"missing world (see '$script_name --help' for usage)"}"
	path="$world_path/$world"
	mkdir -p "$path/"
	if [ ! -f "$path/version" ]; then
		default_version > "$path/version"
	fi
	echo "$path"
}

launch() {
	local which config log tail_pid
	which="$1"
	config="$(realpath "$DRAGONBLOCKS_CONFIG")/$which.conf"
	log="$(realpath "$log_path")/$which-$(timestamp).log"
	shift

	touch "$log"
	tail -f "$log" >&2 &
	tail_pid="$?"

	cd "$version_working_dir"
	"${version_exec_dir}dragonblocks-$which" --config "$config" "$@" 2> "$log"
	kill "$tail_pid"
}

unlaunch() {
	pkill -g 0 -f "dragonblocks-$1"
}

server_ipc() {
	while read -r cmd args; do
		case "${cmd}" in
			listen)
				echo "$args" > "$1"
				;;
		esac
	done
}

case $1 in
	help|--help)
		cat << EOF
Synopsis: $script_name COMMAND

Available commands:
	help
		print this help text

	singleplayer WORLD [PLAYER] [ADDRESS]
		launch a singleplayer instance playing WORLD. if present, internal server
		will listen to ADDRESS. if PLAYER is not present, "singleplayer" will be used

	client PLAYER ADDRESS
		launch client and connect it to server at ADDRESS. PLAYER is used as player name

	server WORLD ADDRESS
		launch server playing WORLD listening to ADDRESS

	worlds
		list available worlds including their path and version

	version
		print default version used for creating new worlds (server and singleplayer)
		and connecting to servers (client). this is either the manually selected default version
		or, if not set, the current path if it contains a dragonblocks installation,
		or "system" if dragonblocks is installed globally on the system.

	version default VERSION
		change default version

	version list
		list installed versions

	version download [VERSION]
		downloads and installs VERSION from the update server ($DRAGONBLOCKS_URL),
		and sets it as default version. if VERSION is not present, the latest snapshot
		will be downloaded

	version check
		print information about versions available on the update server
EOF

		;;

	singleplayer)
		world="$(get_world "$2")"
		load_version "$(cat "$world/version")"

		addrfile="$(realpath "$DRAGONBLOCKS_TMP")/address-$(timestamp).fifo"
		mkfifo "$addrfile"

		trap "unlaunch server" INT TERM

		launch server \
			--world "$(realpath "$world")" \
			--ipc \
			"${4:-::1:}" \
			| server_ipc "$addrfile" &

		launch client \
			--screenshot-dir "$(realpath "$screenshot_path")" \
			"${3:-singleplayer}" \
			"$(cat "$addrfile")"

		unlaunch server
		;;

	client)
		load_version "$(default_version)"

		trap "unlaunch client" INT TERM

		launch client \
			--screenshot-dir "$(realpath "$screenshot_path")" \
			"${2:?"missing player (see '$script_name --help')"}" \
			"${3:?"missing address (see '$script_name --help')"}"
		;;

	server)
		world="$(get_world "$2")"
		load_version "$(cat "$world/version")"

		trap "unlaunch server" INT TERM

		launch server \
			--world "$(realpath "$world")" \
			"${3:?"missing address (see '$script_name --help')"}"
		;;

	worlds)
		for world in "$world_path/"*; do
			echo "$(basename "$world" '') on version $(cat "$world/version") at $(realpath "$world")"
		done
		;;

	version)
		case "$2" in
			"")
				default_version
				;;

			default)
				echo "${3:?"missing version (see '$script_name --help')"}" > \
					"$default_version_path"
				;;

			list)
				local_version || true
				system_version || true
				for version in "$version_path"/*; do
					[ -e "$version" ] && basename "$version" ''
				done

				;;

			download)
				echo "not implemented yet"
				;;

			check)
				echo "not implemented yet"
				;;

			*)
				>&2 echo "invalid command, see '$script_name --help'"
				;;
		esac
		;;

	*)
		>&2 echo "invalid command, see '$script_name --help'"
		exit 1
		;;
esac
