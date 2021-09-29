#! /bin/bash
mkdir .build
cp -r * .build/
cd .build/
mkdir build
cd build
if ! (cmake -B . -S ../src -DCMAKE_BUILD_TYPE=Release && make clean && make -j$(nproc)); then
	cd ../..
	rm -rf .build
	exit 1
fi
cp Dragonblocks DragonblocksServer ..
cd ..
rm -rf .git* deps src build BUILDING.md snapshot.sh upload.sh DragonblocksAlpha-*.zip screenshot-*.png
cd ..
mv .build DragonblocksAlpha
VERSION=`git tag --points-at HEAD`
if [[ $VERSION = "" ]]; then
	VERSION=`git rev-parse --short HEAD`
fi
zip -r DragonblocksAlpha-$VERSION.zip DragonblocksAlpha/*
rm -rf DragonblocksAlpha
