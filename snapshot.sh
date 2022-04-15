#!/bin/bash
VERSION=`git tag --points-at HEAD`
if [[ $VERSION = "" ]]; then
	VERSION=`git rev-parse --short HEAD`
fi
DIR=dragonblocks_alpha-$VERSION
mkdir .build
cp -r * .build/
cd .build/
mkdir build
cd build
if ! (cmake -B . -S ../src -DCMAKE_BUILD_TYPE=Release -DRESSOURCE_PATH="\"\"" && make clean && make -j$(nproc)); then
	cd ../..
	rm -rf .build
	exit 1
fi
cp dragonblocks dragonblocks_server ..
cd ..
rm -rf .git* deps src build BUILDING.md snapshot.sh upload.sh dragonblocks_alpha-* screenshot-*.png
cd ..
mv .build $DIR
zip -r $DIR.zip $DIR/*
rm -rf $DIR
