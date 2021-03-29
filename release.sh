#! /bin/bash
mkdir .build
cp -r * .build/
cd .build/src
make clobber && make all RELEASE=TRUE -j$(nproc) && make clean
cp Dragonblocks DragonblocksServer ..
cd ..
rm -rf .git* deps src BUILDING.md release.sh DragonblocksAlpha-*.zip
cd ..
mv .build DragonblocksAlpha
RELEASE=`git tag --points-at HEAD`
if [[ $RELEASE = "" ]]; then
	RELEASE=`git rev-parse --short HEAD`
fi
zip -r DragonblocksAlpha-$RELEASE DragonblocksAlpha/*
rm -rf DragonblocksAlpha
