#!/bin/bash

# script for easier execution of libtool-linked dlvhex binary with valgrind
# just call this script as you would call dlvhex

cat dlvhex2 |sed 's/exec "\$progdir/exec valgrind --leak-check=yes --num-callers=25 "\$progdir/' >valgrinddlvhex2
chmod a+x valgrinddlvhex2
./valgrinddlvhex2 $*

