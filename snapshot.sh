#!/bin/bash
set -e

VERSION=`git tag --points-at HEAD`
if [[ $VERSION = "" ]]; then
	VERSION=`git rev-parse --short HEAD`
fi

BUILD=build-release
SNAPSHOT=dragonblocks_alpha-$VERSION
TOOLCHAIN=
DOTEXE=
DOTSH=".sh"
if [[ "$1" == "mingw" ]]; then
	BUILD=build-mingw
	SNAPSHOT=dragonblocks_alpha-win64-$VERSION
	TOOLCHAIN=mingw.cmake
	DOTEXE=".exe"
	DOTSH=".bat"
fi

mkdir -p $BUILD

cmake -B $BUILD -S src \
	-DCMAKE_BUILD_TYPE="Release" \
	-DASSET_PATH="assets/" \
	-DCMAKE_C_FLAGS="-Ofast" \
	-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"

make --no-print-directory -C $BUILD -j$(nproc)

rm -rf $SNAPSHOT
mkdir $SNAPSHOT

cp -r \
	assets \
	$BUILD/dragonblocks_client$DOTEXE \
	$BUILD/dragonblocks_server$DOTEXE \
	singleplayer$DOTSH \
	LICENSE \
	README.md \
	$SNAPSHOT

zip -r $SNAPSHOT.zip $SNAPSHOT/*
