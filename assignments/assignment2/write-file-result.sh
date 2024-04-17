#!/bin/sh

touch fileresult.txt

make clean -C ../../finder-app/
make CROSS_COMPILE=aarch64-none-linux-gnu- -C ../../finder-app/

file ../../finder-app/writer > fileresult.txt
