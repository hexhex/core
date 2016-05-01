#!/bin/bash -e


################################################################################
# build-windows.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# TODO
#
################################################################################


################################################################################
# Configuration parameters

ZLIB_VERSION="1.2.8"
OPENSSL_VERSION="1.0.2g"
CURL_VERSION="7.46.0"
BZIP_VERSION="1.0.6"
BOOST_VERSION="1.57.0" # 1.59.0 is not working for cc
PYTHON_VERSION="2.7"

CORE_CLONE_URL="https://github.com/hexhex/core.git"

ROOT_DIR=`pwd`
OUTPUT_IO=/dev/null
BUILD_DIR=$ROOT_DIR/build-windows
LIB_DIR=$BUILD_DIR/out
PYTHON_DIR=$BUILD_DIR/python/mingw32/opt
MINGW_DIR=/usr/i686-w64-mingw32


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

  deps="build-essential autoconf libtool pkg-config scons bison re2c p7zip-full python-dev libxml2-dev libxslt1-dev git ccze mingw-w64"
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

function buildZLib {
  if [ ! -d "$BUILD_DIR/zlib" ]; then
    cd $BUILD_DIR

    echo "==> Downloading zlib $ZLIB_VERSION"

    wget http://zlib.net/zlib-$ZLIB_VERSION.tar.gz &> $OUTPUT_IO
    tar xzf zlib-$ZLIB_VERSION.tar.gz &> $OUTPUT_IO
    mv zlib-$ZLIB_VERSION zlib
    rm zlib-$ZLIB_VERSION.tar.gz
    cd zlib

    echo "==> Building zlib libraries"

    mingw ./configure &> $OUTPUT_IO
    mingw make libz.a &> $OUTPUT_IO

    # Manually install (requird libc for exe not available with mingw-w64):
    mkdir -p $LIB_DIR/include &> $OUTPUT_IO
    mkdir -p $LIB_DIR/lib &> $OUTPUT_IO
    cp zconf.h zlib.h $LIB_DIR/include &> $OUTPUT_IO
    cp libz.a $LIB_DIR/lib &> $OUTPUT_IO
  fi
}

function buildOpenSsl {
  if [ ! -d "$BUILD_DIR/openssl" ]; then
    cd $BUILD_DIR

    echo "==> Downloading OpenSSL $OPENSSL_VERSION"

    wget https://www.openssl.org/source/openssl-$OPENSSL_VERSION.tar.gz &> $OUTPUT_IO
    tar xzf openssl-$OPENSSL_VERSION.tar.gz &> $OUTPUT_IO
    mv openssl-$OPENSSL_VERSION openssl
    rm openssl-$OPENSSL_VERSION.tar.gz
    cd openssl

    echo "==> Building OpenSSL libraries"

    mingw ./Configure --openssldir=$LIB_DIR mingw &> $OUTPUT_IO
    mingw make  &> $OUTPUT_IO
    mingw make install  &> $OUTPUT_IO
  fi
}

function buildCurl {
  # Compile curl
  if [ ! -d "$BUILD_DIR/curl" ]; then
    cd $BUILD_DIR

    echo "==> Downloading Curl $CURL_VERSION"

    wget http://curl.haxx.se/download/curl-$CURL_VERSION.tar.gz &> $OUTPUT_IO
    tar xzf curl-$CURL_VERSION.tar.gz &> $OUTPUT_IO
    mv curl-$CURL_VERSION curl
    rm curl-$CURL_VERSION.tar.gz
    cd curl

    echo "==> Building Curl libraries"

    ./configure --host=i686-w64-mingw32 --prefix=$LIB_DIR --enable-static=yes --enable-shared=no --with-zlib=$LIB_DIR --with-ssl=$LIB_DIR &> $OUTPUT_IO
    make &> $OUTPUT_IO
    make install &> $OUTPUT_IO
  fi
}

function buildBZip {
  # Compile bzip
  if [ ! -d "$BUILD_DIR/bzip2" ]; then
    cd $BUILD_DIR

    echo "==> Downloading bzip $BZIP_VERSION"

    wget http://www.bzip.org/$BZIP_VERSION/bzip2-$BZIP_VERSION.tar.gz
    tar xzf bzip2-$BZIP_VERSION.tar.gz &> $OUTPUT_IO
    mv bzip2-$BZIP_VERSION bzip2
    rm bzip2-$BZIP_VERSION.tar.gz
    cd bzip2

    echo "==> Building bzip libraries"

    mingw make &> $OUTPUT_IO
    mingw make install PREFIX=$LIB_DIR &> $OUTPUT_IO
  fi
}

function installPython {
  if [ ! -d "$BUILD_DIR/python" ]; then
    cd $BUILD_DIR
    echo "==> Downloading Python 2.7"
    wget https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/4.9.2/threads-posix/dwarf/i686-4.9.2-release-posix-dwarf-rt_v3-rev1.7z
    # Extract only python things
    7z x i686-4.9.2-release-posix-dwarf-rt_v3-rev1.7z -opython mingw32/opt/include/python2.7 mingw32/opt/bin/python* -r
    rm i686-4.9.2-release-posix-dwarf-rt_v3-rev1.7z
    # Patch python-config script, '-ne' flag is not defined for mingw-w64 cross compiler
    sed -i '/echo -ne $RESULT/c\echo $RESULT' $PYTHON_DIR/bin/python-config.sh
    chmod +x $PYTHON_DIR/bin/python-config.sh
    # TODO: Look for solution without sudo command, this is only needed for boost.m4 that uses python-config
    sudo rm -f /usr/local/bin/python-config
    sudo ln -s $PYTHON_DIR/bin/python-config.sh /usr/local/bin/python-config

    # Python mingw dll.a file for official python installation (the one from mingw toolchain requires other dependencies)
    wget https://bitbucket.org/carlkl/mingw-w64-for-python/downloads/libpython-cp27-none-win32.7z
    7z x libpython-cp27-none-win32.7z
    mv libs/libpython27.dll.a $LIB_DIR/lib/libpython2.7.a
    rm -rf libs libpython-cp27-none-win32.7z
  fi
}

# Boost is built standalone and not used from the package manager
function buildBoost {
  # TODO: Better check is to look for libs and not for source dir
  if [ ! -d "$BUILD_DIR/boost" ]; then
    cd $BUILD_DIR

    echo "==> Downloading Boost $BOOST_VERSION"

    boost_file_version=`echo $BOOST_VERSION | sed -e "s/\./_/g"`

    wget http://sourceforge.net/projects/boost/files/boost/$BOOST_VERSION/boost_$boost_file_version.tar.gz &> $OUTPUT_IO
    tar xzf boost_$boost_file_version.tar.gz &> $OUTPUT_IO
    mv boost_$boost_file_version boost
    rm boost_$boost_file_version.tar.gz
    cd boost

    echo "==> Building Boost libraries"

    # We set Python version manually, so disable in that step to prevent an entry in project-config.jam
    ./bootstrap.sh -without-libraries=python  &> $OUTPUT_IO
    # Set cross compiler and custom python path
    echo "using gcc : 4.8 : i686-w64-mingw32-g++ ;" > user-config.jam
    echo "using python : 2.7 : $PYTHON_DIR/bin/python.exe : $PYTHON_DIR/include/python2.7 : $PYTHON_DIR/lib ;" >> user-config.jam
    ./b2 -q --user-config=user-config.jam toolset=gcc target-os=windows threading=multi threadapi=win32 variant=release link=static runtime-link=static --without-context --without-coroutine install --prefix=$LIB_DIR include=$LIB_DIR/include  &> $OUTPUT_IO
  fi
}

function buildCore {
  # Compile core
  if [ ! -d "$BUILD_DIR/core" ]; then
    cd $BUILD_DIR

    echo "==> Downloading DLVHEX"

    git clone --recursive $CORE_CLONE_URL &> $OUTPUT_IO
    cd core

    echo "==> Building DLVHEX"

    ./bootstrap.sh &> $OUTPUT_IO

    # Set Python configs
    export PYTHON_BIN=python2.7.exe
    export PYTHON_INCLUDE_DIR=$PYTHON_DIR/include/python2.7
    export PYTHON_LIB=python2.7

    # Patch the gringo patch for cross-compiling
    sed -i "1s|^|62c62\n< env['CXX']            = 'g++'\n---\n> env['CXX']            = 'i686-w64-mingw32-g++'\n68c68\n< env['LIBPATH']        = []\n---\n> env['LIBPATH']        = ['$LIB_DIR/lib', '$MINGW_DIR/libs']\n|" buildclaspgringo/SConstruct.patch

    # see http://curl.haxx.se/docs/faq.html#Link_errors_when_building_libcur about CURL_STATICLIB define
    mingw ./configure LDFLAGS="-static -static-libgcc -static-libstdc++ -L$LIB_DIR/lib -L$PYTHON_DIR/lib/python2.7/config/" --prefix $LIB_DIR PKG_CONFIG_PATH=$LIB_DIR/lib/pkgconfig LOCAL_PLUGIN_DIR=plugins --enable-python --enable-static --disable-shared --enable-static-boost --with-boost=$LIB_DIR --with-libcurl=$LIB_DIR --host=i686-w64-mingw32 CFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB" CXXFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB" CPPFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB"
    mingw make
    mingw make install PREFIX=$LIB_DIR
  fi
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

# Create the xc env and enable it
mkdir -p $LIB_DIR/bin
cat >$LIB_DIR/bin/mingw << 'EOF'
#!/bin/sh
PREFIX=i686-w64-mingw32
export CC=$PREFIX-gcc
export CXX=$PREFIX-g++
export CPP=$PREFIX-cpp
export RANLIB=$PREFIX-ranlib
export PATH="/usr/$PREFIX/bin:$PATH"
exec "$@"
EOF
chmod u+x $LIB_DIR/bin/mingw
export PATH="$LIB_DIR/bin:$PATH"

# Create build dir
prepare

buildZLib
buildOpenSsl
buildCurl
buildBZip
installPython
buildBoost

buildCore

# cleanup

echo "==> Build finished"
