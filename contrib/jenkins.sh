#!/bin/bash

set -e

BUILDS=""
BUILDS+="simtrace/dfu simtrace/cardem " # simtrace/trace simtrace/triple_play
BUILDS+="qmod/dfu qmod/cardem "
BUILDS+="owhw/dfu owhw/cardem "

cd firmware

for build in $BUILDS; do
	board=`echo $build | cut -d "/" -f 1`
	app=`echo $build | cut -d "/" -f 2`
	echo
	echo "=============== $board / $app START  =============="
	make BOARD="$board" APP="$app"
	echo "=============== $board / $app RES:$? =============="
done
