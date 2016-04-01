#!/bin/bash

################################################################################
# build-linux.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# Run this script to create a static build of the DLVHEX project for Linux
# platforms. The script clones a clean copy of the repositories and builds the
# binaries. Tested on Ubuntu and Debian.
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
BUILD_DIR=$ROOT_DIR/build-linux
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

  deps="sed git build-essential autoconf autotools-dev libtool wget scons bison re2c python-dev libpython-all-dev libcurl4-openssl-dev libbz2-dev"
  missing_deps=""

  for dep in `echo $deps`; do
    if ! dpkg -s $dep &> /dev/null; then
      missing_deps="$missing_deps $dep"
    fi
  done

  if [ ! -z "$missing_deps" ]; then
    # TODO: Check for other distros and replace apt-get command
    echo "Error: Missing build dependencies, use:"
    echo "apt-get install$missing_deps"
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
  # PIC needed for linking shared plugins with static libs:
  # http://stackoverflow.com/a/19768349/272089
  # http://stackoverflow.com/q/27848105/272089
  ./b2 -q cxxflags=-fPIC link=static runtime-link=static install &> $OUTPUT_IO
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

  # Configure build
  ./bootstrap.sh &> $OUTPUT_IO
  ./configure --prefix $LIB_DIR CXXFLAGS=-fPIC PKG_CONFIG_PATH=$LIB_DIR/lib/pkgconfig LOCAL_PLUGIN_DIR=plugins --enable-python --enable-shared=no --enable-static-boost --with-boost=$LIB_DIR &> $OUTPUT_IO

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
