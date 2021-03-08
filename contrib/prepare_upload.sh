#!/bin/sh -e
# Create copies of binaries with -latest, -$GIT_VERSION (OS#4413, OS#3452)
cd "$(dirname "$0")/.."

GIT_VERSION="$(./git-version-gen .tarball-version)"

echo "Copying binaries with "-latest" and "-$GIT_VERSION" appended..."

cd firmware/bin
for ext in bin elf; do
	for file in *."$ext"; do
		without_ext="${file%.*}"
		cp -v "$file" "$without_ext-latest.$ext"
		cp -v "$file" "$without_ext-$GIT_VERSION.$ext"
	done
done
