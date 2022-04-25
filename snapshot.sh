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
FLAGS="-Ofast"
if [[ "$1" != "" ]]; then
	BUILD=build-$1
	SNAPSHOT=dragonblocks_alpha-$1-$VERSION
	TOOLCHAIN=$1.cmake
	DOTEXE=".exe"
	DOTSH=".bat"
	FLAGS="$FLAGS -static"
fi

mkdir -p $BUILD

cmake -B $BUILD -S src \
	-DCMAKE_BUILD_TYPE="Release" \
	-DASSET_PATH="assets/" \
	-DCMAKE_C_FLAGS="$FLAGS" \
	-DCMAKE_CXX_FLAGS="$FLAGS" \
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
