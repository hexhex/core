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
CURL_VERSION="7.41.0"
BZIP_VERSION="1.0.6"
BOOST_VERSION="1.57.0" # 1.59.0 is not working for cc
PYTHON_VERSION="2.7"

CORE_CLONE_URL="https://github.com/hexhex/core.git"

ROOT_DIR=`pwd`
OUTPUT_IO=/dev/null
BUILD_DIR=$ROOT_DIR/build-windows
LIB_DIR=$BUILD_DIR/out
PYTHON_DIR=$HOME/.wine/drive_c/Python27
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

  deps="build-essential autoconf libtool pkg-config scons bison re2c p7zip-full python-dev libxml2-dev libxslt1-dev git ccze mingw-w64 wine"
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
    mkdir -p $LIB_DIR/usr/local/include &> $OUTPUT_IO
    mkdir -p $LIB_DIR/usr/local/lib &> $OUTPUT_IO
    cp zconf.h zlib.h $LIB_DIR/usr/local/include &> $OUTPUT_IO
    cp libz.a $LIB_DIR/usr/local/lib &> $OUTPUT_IO
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

    mingw ./Configure --openssldir=$LIB_DIR/usr/local mingw &> $OUTPUT_IO
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

    ./configure --host=i686-w64-mingw32 --prefix=$LIB_DIR/usr/local --enable-static=yes --enable-shared=no --with-zlib=$LIB_DIR/usr/local --with-ssl=$LIB_DIR/usr/local &> $OUTPUT_IO
    # TODO: This fails due to crypto and ssl libs after gdi32 lib statement in LIBS from configure
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
    mingw make install PREFIX=$LIB_DIR/usr/local &> $OUTPUT_IO
  fi
}

function installPython {
  # Install Python with Wine
  if [ ! -d "$HOME/.wine/drive_c/Python27" ]; then
    # Python installer
    cd $HOME
    wget https://www.python.org/ftp/python/2.7.11/python-2.7.11.msi
    wine msiexec /i python-2.7.11.msi /L*v log.txt
    # Python patch cause of redefinition of hypit function for MSVC
    sed -i '/#define hypot _hypot/d' $HOME/.wine/drive_c/Python27/include/pyconfig.h
    # Python mingw dll.a file
    wget https://bitbucket.org/carlkl/mingw-w64-for-python/downloads/libpython-cp27-none-win32.7z
    7z e libpython-cp27-none-win32.7z
    rm -rf libs libpython-cp27-none-win32.7z
    # TODO: Try to not use sudo in the script
    sudo mv libpython27.dll.a /usr/i686-w64-mingw32/lib/libpython27.a
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
    echo "using python : 2.7 : /usr/bin/python : $HOME/.wine/drive_c/Python27/include : $LIB_DIR/usr/local/lib ;" >> user-config.jam
    ./b2 -q --user-config=user-config.jam toolset=gcc target-os=windows threading=multi threadapi=win32 variant=release link=static runtime-link=static --without-context --without-coroutine install --prefix=$LIB_DIR/usr/local include=$LIB_DIR/usr/local/include  &> $OUTPUT_IO
  fi
}

function buildCore {
  # Compile core
  if [ ! -d "$BUILD_DIR/core" ]; then
    cd $BUILD_DIR
    git clone --recursive $CORE_CLONE_URL &> $OUTPUT_IO
    cd core
    exit
    ./bootstrap.sh &> $OUTPUT_IO

    # Set Python configs
    export PYTHON_BIN=python27
    export PYTHON_INCLUDE_DIR=$HOME/.wine/drive_c/Python27/include

    # Patch the gringo patch for cross-compiling
    sed -i "1s|^|62c62\n< env['CXX']            = 'g++'\n---\n> env['CXX']            = 'i686-w64-mingw32-g++'\n68c68\n< env['LIBPATH']        = []\n---\n> env['LIBPATH']        = ['$LIB_DIR/usr/local/lib', '$MINGW_DIR/libs']\n|" buildclaspgringo/SConstruct.patch

    # see http://curl.haxx.se/docs/faq.html#Link_errors_when_building_libcur about CURL_STATICLIB define
    mingw ./configure LDFLAGS="-static -static-libgcc -static-libstdc++ -L$LIB_DIR/usr/local/lib" --prefix $LIB_DIR/usr/local PKG_CONFIG_PATH=$LIB_DIR/usr/local/lib/pkgconfig --enable-python --enable-static --disable-shared --enable-static-boost --with-boost=$LIB_DIR/usr/local --with-libcurl=$LIB_DIR/usr/local --host=i686-w64-mingw32 CFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB" CXXFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB" CPPFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB"
    mingw make
    mingw make install PREFIX=$LIB_DIR/usr/local
  fi
}

function cleanup {
  echo "==> Cleaning up build"

  # TODO: Check if directories really exist

  # Clean up boost
  # cd $BUILD_DIR/boost
  # ./b2 --clean &> $OUTPUT_IO

  # Clean up core
  # cd $BUILD_DIR/core
  # make clean &> $OUTPUT_IO
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

# Needed on 64bit hosts to install wine package
sudo dpkg --add-architecture i386

checkDependencies

# Create the xc env and enable it
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

# Override python-config script for proper Windows xc env
echo "cat >/usr/local/bin/python-config << 'EOF'
#!/bin/sh

while [ \"\$1\" != \"\" ]; do
  case \$1 in
    --includes )                  echo \" -I$PYTHON_DIR/include\"
                                  ;;
    --prefix | --exec-prefix )    echo \"$PYTHON_DIR/include\"
                                  ;;
    --cflags )                    echo \" -I$PYTHON_DIR/include -I$MINGW_DIR/include -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 -DG_OS_WIN32\"
                                  ;;
    --libs )                      echo \" -lpthread -mthreads -lm -lpython27 -L$PYTHON_DIR/libs -L$MINGW_DIR/libs\"
                                  ;;
    --ldflags )                   echo \" -lpthread -mthreads -lm -lpython27 -L$PYTHON_DIR/Python27/libs -L$MINGW_DIR/libs -Xlinker -export-dynamic\"
                                  ;;
  esac
  shift
done

EOF" | sudo -s
sudo chmod +x /usr/local/bin/python-config

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
