#!/bin/bash


################################################################################
# build-windows-plugins.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# TODO
#
################################################################################


################################################################################
# Configuration parameters

BASE_CLONE_URL="https://github.com/hexhex/"
# TODO: Allow to override from command line to build a specific plugin only
PLUGINS="nestedhexplugin mcsieplugin actionplugin actionplugin-addons stringplugin"
# Plugins TODO: dlliteplugin, caspplugin, wordnetplugin
# Don't change the following ones, should match directories from core build
ROOT_DIR=`pwd`
OUTPUT_IO=/dev/null
BUILD_DIR=$ROOT_DIR/build-windows
LIB_DIR=$BUILD_DIR/out

# Color output
C_WHITE='\033[1;37m'
C_GREEN='\033[0;32m'
C_RED='\033[0;31m'
C_YELLOW='\033[1;33m'
C_NC='\033[0m'
T_ERROR="${C_RED}Error${C_NC}:"
T_WARN="${C_YELLOW}Warning${C_NC}:"
T_INFO="${C_WHITE}Info${C_NC}:"
T_DONE="${C_GREEN}Done${C_NC}:"



################################################################################
# Helper functions

function usage {
  echo -e "usage: $0 [-v -h]"
  echo -e
  echo -e "Parameters:"
  echo -e "  -h     print this help"
  echo -e "  -v     Enable verbose output"
}

function prepare {
  echo -e "==> Create build directory"
  mkdir -p $BUILD_DIR

  # Create the xc env and enable it
  echo -e "==> Set up cross-compilation environment"
  mkdir -p $HOME/bin
  cat >$HOME/bin/mingw << 'EOF'
#!/bin/sh
PREFIX=i686-w64-mingw32
export CC=$PREFIX-gcc
export CXX=$PREFIX-g++
export CPP=$PREFIX-cpp
export RANLIB=$PREFIX-ranlib
export PATH="/usr/$PREFIX/bin:$PATH"
exec "$@"
EOF
  chmod u+x $HOME/bin/mingw
  export PATH="$HOME/bin:$PATH"
}

# Check for missing depdencies
function checkDependencies {
  echo -e "==> Check build dependencies"

  # TODO: Check for missing libs
}

function processPlugin {
  # Name of currently built plugin
  plugin_name=$1

  echo -e "==> ${T_INFO} Process <$plugin_name> repository"

  cd $BUILD_DIR

  # Check if directory already exists
  if [ -d $plugin_name ]; then
    echo -e "===> ${T_WARN} Source directory already exists"
    # Do not delete already existing repository but re-use it
    # rm -rf $plugin_name
  else
    echo -e "===> Clone repository"

    # Clone latest sources
    plugin_url="${BASE_CLONE_URL}${plugin_name}.git"
    git clone --recursive $plugin_url &> $OUTPUT_IO
    if [ $? -ne 0 ]; then
      echo -e "===> ${T_ERROR} Can't clone repository from url: <$plugin_url>"
      exit 1
    fi
  fi

  cd $plugin_name

  # Check if there exists a bootstrap file, if not try to iterate over subdirs,
  # needed for plugins like actionplugin-addons
  if [ ! -f bootstrap.sh ]; then
    echo -e "===> ${T_INFO} No bootstrap.sh file found in root directory, look for subdirectories"

    for subdir in */ ; do

      if [ -d $subdir ]; then
        cd $subdir
        subdir_name=${subdir%/}
        buildPlugin $subdir_name
        cd ..
      fi
    done

  # bootstrap.sh file found, build
  else
    buildPlugin $plugin_name
  fi
}

# Make sure to dir to plugin directory before calling this method
function buildPlugin {
  # Name of currently built plugin
  plugin_name=$1

  echo -e "===> ${T_INFO} Build <$plugin_name> plugin"

  if [ -f bootstrap.sh ]; then
    echo -e "===> Configure"

    # Configure build
    ./bootstrap.sh &> $OUTPUT_IO
    mingw ./configure --enable-static=no --enable-shared=yes --prefix $LIB_DIR/usr/local PKG_CONFIG_PATH=$LIB_DIR/usr/local/lib/pkgconfig --with-boost=$LIB_DIR/usr/local --host=i686-w64-mingw32  &> $OUTPUT_IO

    echo -e "===> Build"

    # Build plugin
    mingw make LDFLAGS="-L$LIB_DIR/usr/local/lib -lboost_system -ldlvhex2-base -no-undefined" &> $OUTPUT_IO
    if [ $? -ne 0 ]; then
      echo -e "===> ${T_ERROR} Error building library"
    else
      mingw make install &> $OUTPUT_IO

      # TODO: Run make check?

      echo -e "===> ${T_DONE} Build finished"
    fi

    # Clean up
    make clean &> $OUTPUT_IO
  else
    echo -e "===> ${T_WARN} No bootstrap.sh file found, skipping build"
  fi
}

# Collects build artifacts used for distribution later on
function collect {
  echo -e "==> Distributing build files"
  # TODO: Where to copy build?
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

# Build plugins
# Iterate over plugins to be built
for plugin in `echo -e $PLUGINS`; do
  processPlugin $plugin
done

echo -e "==> ${T_DONE} All builds finished"
