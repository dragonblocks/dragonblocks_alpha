#!/bin/sh
set -e

: "${URL:=https://dragonblocks.lizzy.rs/upload.php}"
name="$(git describe --tags)"
path="@snapshots/dragonblocks_alpha-$name"
ref="$(git tag --points-at HEAD)"
release="1"
files=""

if [ "$ref" = "" ]; then
	ref="$(git rev-parse --short HEAD)"
	release="0"
fi

for arg; do
	suffix="$(echo "$arg" | tr '[:upper:]' '[:lower:]')"
	files="$files -F $arg=$path-$suffix.zip"
done

# shellcheck disable=SC2086
curl -L --fail-with-body -i -X POST \
	-H "Content-Type: multipart/form-data" \
	-F "secret=$SECRET" \
	-F "name=$name" \
	-F "ref=$ref" \
	-F "release=$release" \
	$files \
	"$URL"
