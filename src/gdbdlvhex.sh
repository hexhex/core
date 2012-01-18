#!/bin/bash

# script for easier execution of libtool-linked dlvhex binary with gdb
# just call this script as you would call gdb
# if you don't want to run it immediately, remove the "-ex run " below

cat dlvhex |sed 's/exec "\$progdir/exec gdb -ex run --args "\$progdir/' >gdbdlvhex
chmod a+x gdbdlvhex
./gdbdlvhex $*

