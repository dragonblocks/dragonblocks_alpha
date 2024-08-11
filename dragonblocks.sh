#!/bin/sh
# dragonblocks launcher script
set -e

script_name="$0"

: "${DRAGONBLOCKS_DATA:=${XDG_DATA_HOME:-$HOME/.local/share}/dragonblocks_alpha}"
: "${DRAGONBLOCKS_CONFIG:=${XDG_CONFIG_HOME:-$HOME/.config}/dragonblocks_alpha}"
: "${DRAGONBLOCKS_TMP:=${XDG_RUNTIME_DIR:-/tmp}/dragonblocks_alpha}"
: "${DRAGONBLOCKS_LOG:=${XDG_STATE_HOME:-$HOME/.local/state}/dragonblocks_alpha}"

mkdir -p "$DRAGONBLOCKS_DATA/"
mkdir -p "$DRAGONBLOCKS_CONFIG/"
mkdir -p "$DRAGONBLOCKS_TMP/"
mkdir -p "$DRAGONBLOCKS_LOG/"

: "${DRAGONBLOCKS_URL:=https://dragonblocks.lizzy.rs}"

load_version() {
	case "$1" in
		"system")
			version_exec_dir=""
			version_working_dir="."
			;;

		"local")
			version_exec_dir="./"
			version_working_dir="."
			;;

		*)
			version_exec_dir="./"
			version_working_dir="$DRAGONBLOCKS_DATA/versions/$1"
			;;
	esac
}

timestamp() {
	date +%Y-%m-%d-%H-%M-%S
}

default_version() {
	if [ -n "$DRAGONBLOCKS_VERSION" ]; then
		echo "$DRAGONBLOCKS_VERSION"
	elif [ -f "$DRAGONBLOCKS_DATA/default_version" ]; then
		cat "$DRAGONBLOCKS_DATA/default_version"
	elif [ -x "./dragonblocks-client" ]; then
		echo "local"
	elif command -v "dragonblocks-client" &>/dev/null; then
		echo "system"
	else
		>&2 echo "no dragonblocks installation found. use '$script_name version download' to obtain versions"
		return 1
	fi
}

get_world() {
	local world="${1:?missing world (see '$script_name --help' for usage)}"
	local path="$DRAGONBLOCKS_DATA/worlds/$world"
	mkdir -p "$path/"
	if [ ! -f "$path/version" ]; then
		default_version > "$path/version"
	fi
	echo "$path"
}

launch() {
	local which="$1"
	local config="$(realpath $DRAGONBLOCKS_CONFIG/$which.conf)"
	local log="$(realpath $DRAGONBLOCKS_LOG/$which-$(timestamp).log)"

	shift
	cd "$version_working_dir"
	stdbuf -o0 "${version_exec_dir}dragonblocks-$which" --config "$config" $@ |& tee -i "$log"
}

unlaunch() {
	pkill -g 0 -f dragonblocks-$1
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
		or, if not set, "local" if the local directory contains a dragonblocks installation,
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
		world="$(get_world $2)"
		load_version "$(<$world/version)"

		addrfile="$(realpath $DRAGONBLOCKS_TMP/address-$(timestamp).fifo)"
		mkfifo "$addrfile"

		trap "unlaunch server" SIGINT SIGTERM

		launch server --world "$(realpath $world)" --write-address "$addrfile" "${4:-::1:}" &
		launch client "${3:-singleplayer}" "$(<$addrfile)"

		unlaunch server
		;;

	client)
		load_version "$(default_version)"

		trap "unlaunch client" SIGINT SIGTERM

		launch client "${2:?missing player (see '$script_name --help')}" \
			"${3:?missing address (see '$script_name --help')}"
		;;

	server)
		world="$(get_world $2)"
		load_version "$(<$world/version)"

		trap "unlaunch server" SIGINT SIGTERM

		launch server --world "$(realpath $world)" \
			"${3:?missing address (see '$script_name --help')}"
		;;

	worlds)
		mkdir -p "$DRAGONBLOCKS_DATA/worlds/"

		for world in "$DRAGONBLOCKS_DATA/worlds/"*; do
			echo "$(basename $world '') on version $(<$world/version) at $(realpath $world)"
		done
		;;

	version)
		mkdir -p "$DRAGONBLOCKS_DATA/versions/"

		case "$2" in
			"")
				default_version
				;;

			default)
				echo "${3:?missing version (see '$script_name --help')}" > \
					"$DRAGONBLOCKS_DATA/default_version"
				;;

			list)
				for version in "$DRAGONBLOCKS_DATA/versions/"*; do
					echo "$(basename $version '')"
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
