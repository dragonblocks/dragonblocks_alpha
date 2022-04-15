#!/bin/bash
VERSION=`git tag --points-at HEAD`
IS_RELEASE="1"
if [[ $VERSION = "" ]]; then
	VERSION=`git rev-parse --short HEAD`
	IS_RELEASE="0"
fi
curl -f -i -X POST -H "Content-Type: multipart/form-data" \
	-F "secret=$SECRET" \
	-F "name=$VERSION" \
	-F "is_release=$IS_RELEASE" \
	-F "build=@dragonblocks_alpha-$VERSION.zip" \
	https://elidragon.tk/dragonblocks_alpha/upload.php
