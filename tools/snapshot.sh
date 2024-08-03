#!/bin/bash
set -e

version="$(git describe --tags)"

mkdir -p "snapshots"

build="build"
snapshot="dragonblocks_alpha-$version"
dotexe=""
dotsh=".sh"
crossfile=""

if [[ "$1" != "" ]]; then
	build="$build-$1"
	snapshot="$snapshot-$1"
	toolchain="$1.cmake"
	dotexe=".exe"
	dotsh=".bat"
	crossfile="--cross-file=tools/$1-toolchain.txt"

	export CFLAGS="$CFLAGS -static"
fi

meson setup "snapshots/$build" \
	-Dbuildtype=release \
	-Doptimization=2 \
	-Dasset_path="assets/" \
	-Ddefault_library=static \
	-Dfreetype2:harfbuzz=disabled \
	-Dfreetype2:bzip2=disabled \
	-Dfreetype2:brotli=disabled \
	-Dfreetype2:png=disabled \
	$crossfile \
	--wrap-mode=forcefallback \
	--reconfigure

cd "snapshots"

meson compile -C "$build"

rm -rf "$snapshot"
mkdir "$snapshot"

cp -r \
	"../assets" \
	"$build/dragonblocks_client$dotexe" \
	"$build/dragonblocks_server$dotexe" \
	"../singleplayer$dotsh" \
	"../LICENSE" \
	"../README.md" \
	"$snapshot"

rm -f "$snapshot.zip"
zip -r "$snapshot.zip" "$snapshot"/*
