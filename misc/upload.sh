#!/bin/bash
set -e

name="$(git describe --tags)"
path="@snapshots/dragonblocks_alpha-$name"
ref="$(git tag --points-at HEAD)"
release="1"

if [[ "$ref" == "" ]]; then
	ref="$(git rev-parse --short HEAD)"
	release="0"
fi

curl -L -f -i -X POST -H "Content-Type: multipart/form-data" \
	-F "secret=$SECRET" \
	-F "name=$name" \
	-F "ref=$ref" \
	-F "release=$release" \
	-F "Linux=$path.zip" \
	-F "Win32=$path-win32.zip" \
	-F "Win64=$path-win64.zip" \
	https://dragonblocks.lizzy.rs/upload.php
