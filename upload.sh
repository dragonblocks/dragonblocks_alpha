#!/bin/bash
echo "Snapshot uploading temporarily disabled"
exit

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
	-F "ubuntu=@dragonblocks_alpha-$VERSION.zip" \
	-F "windows=@dragonblocks_alpha-win64-$VERSION.zip" \
	https://elidragon.tk/dragonblocks_alpha/upload.php
