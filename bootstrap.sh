#!/bin/sh

LT=`which libtoolize`

if [ -x "$LT" ]; then
    LT=libtoolize
else
    LT=glibtoolize
fi

# copy libltdl into working repository
$LT -c -f --ltdl

# rebuild autotool files
autoreconf -f -i -W all
