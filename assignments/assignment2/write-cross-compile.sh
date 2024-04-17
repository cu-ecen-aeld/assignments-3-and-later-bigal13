#!/bin/sh

touch cross-compile.txt
aarch64-none-linux-gnu-gcc -print-sysroot -v >>cross-compile.txt 2>&1
