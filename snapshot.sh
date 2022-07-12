#!/bin/bash
set -e

version="$(git describe --tags)"

mkdir "snapshot"
cd "snapshot"

build="build"
snapshot="dragonblocks_alpha-$version"
toolchain=""
dotexe=""
dotsh=".sh"
flags="-Ofast"

if [[ "$1" != "" ]]; then
	build="$build-$1"
	snapshot="$snapshot-$1"
	toolchain="$1.cmake"
	dotexe=".exe"
	dotsh=".bat"
	flags="$flags -static"
fi

mkdir "$build"

cmake -B "$build" -S ../src \
	-DCMAKE_BUILD_TYPE="Release" \
	-DASSET_PATH="assets/" \
	-DCMAKE_C_FLAGS="$flags" \
	-DCMAKE_CXX_FLAGS="$flags" \
	-DCMAKE_TOOLCHAIN_FILE="$toolchain"

make --no-print-directory -C "$build" -j"$(nproc)"

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
