#!/bin/bash

# script for easier execution of libtool-linked dlvhex binary with valgrind
# just call this script as you would call dlvhex

cat dlvhex |sed 's/exec "\$progdir/exec valgrind --leak-check=yes "\$progdir/' >valgrinddlvhex
chmod a+x valgrinddlvhex
./valgrinddlvhex $*

