#!/bin/bash

################################################################################
# build-macx.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# Run this script to create a static build of the DLVHEX project for Mac OS X.
#  The script clones a clean copy of the repositories and builds the
# binaries. Tested on Mac OS X 10.10 and above.
#
# The Mac OS X script uses the homebrew package manager to install additional
# dependencies.
#
################################################################################


################################################################################
# Configuration parameters

BOOST_VERSION="1.59.0"
PYTHON_VERSION="2.7"

CORE_CLONE_URL="https://github.com/hexhex/core.git"

# Don't change the following ones
ROOT_DIR=`pwd`
OUTPUT_IO=/dev/null
BUILD_DIR=$ROOT_DIR/build-macx
LIB_DIR=$BUILD_DIR/out
BOOST_FILE_VERSION=`echo $BOOST_VERSION | sed -e "s/\./_/g"`


################################################################################
# Helper functions

function usage {
  echo "usage: $0 [-v -h]"
  echo
  echo "Parameters:"
  echo "  -h     print this help"
  echo "  -v     Enable verbose output"
}

function prepare {
  echo "==> Creating build directory"
  mkdir -p $BUILD_DIR
}

# Check for missing depdencies
function checkDependencies {
  echo "==> Checking build dependencies"

  # TODO: Bison is always found on Mac OS X, so we have to check for min version too!
  deps="git autoconf automake libtool pkg-config wget scons bison re2c python"
  missing_deps=""

  for dep in `echo $deps`; do
    if ! which $dep &> /dev/null; then
      missing_deps="$missing_deps $dep"
    fi
  done

  if [ ! -z "$missing_deps" ]; then
    echo "Error: Missing build dependencies, use:"
    echo "brew install$missing_deps"
    exit 1
  fi
}

# Boost is built standalone and not used from the package manager
function buildBoost {
  cd $BUILD_DIR

  echo "==> Downloading Boost $BOOST_VERSION"

  # Download and extract boost
  wget http://sourceforge.net/projects/boost/files/boost/$BOOST_VERSION/boost_$BOOST_FILE_VERSION.tar.gz &> $OUTPUT_IO
  tar xzf boost_$BOOST_FILE_VERSION.tar.gz &> $OUTPUT_IO
  mv boost_$BOOST_FILE_VERSION boost
  rm boost_$BOOST_FILE_VERSION.tar.gz
  cd boost

  echo "==> Building Boost libraries"

  # Build boost
  ./bootstrap.sh --prefix=$LIB_DIR --with-python-version=$PYTHON_VERSION &> $OUTPUT_IO
  ./b2 -q link=static runtime-link=static --layout=tagged install &> $OUTPUT_IO
}

function buildCore {
  cd $BUILD_DIR

  echo "==> Cloning dlvhex core repository"

  # Clone latest sources
  git clone --recursive $CORE_CLONE_URL &> $OUTPUT_IO
  cd core

  echo "==> Configuring build"

  # Configure build
  export PYTHON_BIN=python$PYTHON_VERSION

  # We need to use bison from homebrew and not the system installed one from Mac OS X
  # TODO: Better solution for that?
  export PATH=/usr/local/Cellar/bison/3.0.4/bin:$PATH

  # Configure build
  # ./bootstrap.sh &> $OUTPUT_IO
  ./configure --prefix $LIB_DIR PKG_CONFIG_PATH=$LIB_DIR/lib/pkgconfig LOCAL_PLUGIN_DIR=plugins --enable-python --enable-shared=no --enable-static-boost --with-boost=$LIB_DIR &> $OUTPUT_IO

  echo "==> Patching Makefile"
  # TODO
  # -> In Makefile: libdlvhex2-base.la may not include$(libdlvhex2_base_la_LIBADD),
  # otherwise boost static libs are added to static lib and it won't link anymore!

  echo "==> Building core binary"

  # Build core
  make &> $OUTPUT_IO
  make install &> $OUTPUT_IO
}

# Collects build artifacts used for distribution later on
function collect {
  echo "==> Distributing build files"
  # TODO: Where to copy build?
}

function cleanup {
  echo "==> Cleaning up build"

  # TODO: Check if directories really exist

  # Clean up boost
  cd $BUILD_DIR/boost
  ./b2 --clean &> $OUTPUT_IO

  # Clean up core
  cd $BUILD_DIR/core
  make clean &> $OUTPUT_IO

  echo "==> Build finished"
}


################################################################################
# Script execution

# Check given parameters
while [ "$1" != "" ]; do
  case $1 in
    -v | --verbose )            OUTPUT_IO=/dev/stdout
                                ;;
    -h | --help )               usage
                                exit
                                ;;
    * )                         usage
                                exit 1
  esac
  shift
done

checkDependencies
prepare

# Build dependencies and library
buildBoost
buildCore

# Cleanup
#cleanup
