#!/bin/bash
set -e

version="$(git describe --tags)"

mkdir -p "snapshots"

build="build"
snapshot="dragonblocks_alpha-$version"
dotexe=""
crossfile=""
launcher="dragonblocks.sh"

if [[ "$1" != "" ]]; then
	build="$build-$1"
	snapshot="$snapshot-$1"
	toolchain="$1.cmake"
	dotexe=".exe"
	launcher="singleplayer.bat"
	crossfile="--cross-file=tools/$1-toolchain.txt"

	export CFLAGS="$CFLAGS -static"
fi

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
