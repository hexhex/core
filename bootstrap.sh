#!/bin/sh
#
# Thomas Krennwallner <tkren@kr.tuwien.ac.at>
#
# 1. check for libtool, autoconf, automake, pkg-config
# 2. run libtoolize and autoreconf to create libltdl, configure, and Makefile.in's
#

LT=`which libtoolize`
GLT=`which glibtoolize`

if [ -x "$LT" ]; then
    LT=libtoolize
elif [ -x "$GLT" ]; then
    LT=glibtoolize
else
    echo "libtoolize: command not found. Please install GNU libtool, GNU autoconf, and GNU automake."
    exit 1
fi

ARC=`which autoreconf`

if [ ! -x "$ARC" ]; then
    echo "autoreconf: command not found. Please install GNU autoconf and GNU automake."
    exit 1
fi

AM=`which automake`

if [ ! -x "$AM" ]; then
    echo "automake: command not found. Please install GNU automake."
    exit 1
fi

PC=`which pkg-config`

if [ ! -x "$PC" ]; then
    echo "pkg-config: command not found. Please install pkg-config."
    exit 1
fi

# copy libltdl into working repository
$LT -c -f --ltdl

# rebuild autotool files
autoreconf -f -i -W all
