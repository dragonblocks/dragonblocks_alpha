#!/bin/sh
set -e

version="$(git describe --tags)"

mkdir -p "snapshots"

build="build-$1"
snapshot="dragonblocks_alpha-$version-$1"

case "$1" in
	win*)
		dotexe=".exe"
		launcher="singleplayer.bat"
		crossfile="--cross-file=misc/$1-toolchain.txt"
		export CFLAGS="$CFLAGS -static"
		;;
	*)
		launcher="dragonblocks.sh"
		;;
esac

# shellcheck disable=SC2086
meson setup "snapshots/$build" \
	-Dbuildtype=release \
	-Doptimization=2 \
	-Dasset_path="assets/" \
	-Ddefault_library=static \
	-Dglew:glu=disabled \
	-Dglew:egl=disabled \
	$crossfile \
	--wrap-mode=forcefallback \
	--reconfigure

cd "snapshots"

meson compile -C "$build"

rm -rf "$snapshot"
mkdir "$snapshot"

cp -r \
	"../assets" \
	"$build/dragonblocks-client$dotexe" \
	"$build/dragonblocks-server$dotexe" \
	"../$launcher" \
	"../LICENSE" \
	"../README.md" \
	"$snapshot"

rm -f "$snapshot.zip"
zip -r "$snapshot.zip" "$snapshot"/*
