#!/bin/bash

TOPDIR=`pwd`

if ! [ -x "$(command -v osmo-build-dep.sh)" ]; then
	echo "Error: We need to have scripts/osmo-deps.sh from http://git.osmocom.org/osmo-ci/ in PATH !"
	exit 2
fi

set -e

base="$PWD"
deps="$base/deps"
inst="$deps/install"
export deps inst

osmo-clean-workspace.sh

mkdir "$deps" || true

osmo-build-dep.sh libosmocore "" '--disable-doxygen --enable-gnutls'

export PKG_CONFIG_PATH="$inst/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="$inst/lib"

BUILDS=""
BUILDS+="simtrace/dfu simtrace/cardem " # simtrace/trace simtrace/triple_play
BUILDS+="qmod/dfu qmod/cardem "
BUILDS+="owhw/dfu owhw/cardem "

cd $TOPDIR/firmware
for build in $BUILDS; do
	board=`echo $build | cut -d "/" -f 1`
	app=`echo $build | cut -d "/" -f 2`
	echo
	echo "=============== $board / $app START  =============="
	make BOARD="$board" APP="$app"
	make BOARD="$board" APP="$app" clean
	echo "=============== $board / $app RES:$? =============="
done

echo
echo "=============== FIRMWARE TESTS ==========="
cd $TOPDIR/firmware/test
make clean
make
./card_emu_test
make clean

echo
echo "=============== HOST START  =============="
cd $TOPDIR/host
make clean
make
make clean

osmo-clean-workspace.sh
