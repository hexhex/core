#!/bin/bash -e


################################################################################
# build-windows.sh
#
# Author: Alex Leutgöb <alexleutgoeb@gmail.com>
#
# TODO
#
################################################################################

LIB_DIR=/vagrant_data/out
PYTHON_DIR=$HOME/.wine/drive_c/Python27
MINGW_DIR=/usr/i686-w64-mingw32

################################################################################

# Needed on 64bit hosts to install wine package
sudo dpkg --add-architecture i386

# Install dependencies
sudo apt-get update
sudo apt-get -y upgrade
sudo apt-get -y install build-essential autoconf libtool pkg-config scons bison re2c p7zip-full python-dev libxml2-dev libxslt1-dev git ccze
sudo apt-get -y install mingw-w64
sudo apt-get -y install wine

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

# Compile libz
if [ ! -d "$HOME/zlib-1.2.8" ]; then
  cd $HOME
  wget http://zlib.net/zlib-1.2.8.tar.gz
  tar xzf zlib-1.2.8.tar.gz
  cd zlib-1.2.8
  mingw ./configure
  mingw make libz.a
  # Manually install (requird libc for exe not available with mingw-w64):
  mkdir -p $LIB_DIR/usr/local/include
  mkdir -p $LIB_DIR/usr/local/lib
  cp zconf.h zlib.h $LIB_DIR/usr/local/include
  cp libz.a $LIB_DIR/usr/local/lib
fi

# Compile OpenSSL
if [ ! -d "$HOME/openssl-1.0.2f" ]; then
  cd $HOME
  wget https://www.openssl.org/source/openssl-1.0.2f.tar.gz
  tar xzf openssl-1.0.2f.tar.gz
  cd openssl-1.0.2f
  mingw ./Configure --openssldir=$LIB_DIR/usr/local mingw
  mingw make
  mingw make install
fi

# Compile curl
if [ ! -d "$HOME/curl-7.41.0" ]; then
  cd $HOME
  wget http://curl.haxx.se/download/curl-7.41.0.tar.gz
  tar xzf curl-7.41.0.tar.gz
  cd curl-7.41.0
  ./configure --host=i686-w64-mingw32 --prefix=$LIB_DIR/usr/local --enable-static=yes --enable-shared=no --with-zlib=$LIB_DIR/usr/local --with-ssl=$LIB_DIR/usr/local
  # TODO: This fails due to crypto and ssl libs after gdi32 lib statement in LIBS from configure
  make
  make install
fi

# Compile bzip
if [ ! -d "$HOME/bzip2-1.0.6" ]; then
  cd $HOME
  wget http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz
  tar xzf bzip2-1.0.6.tar.gz
  cd bzip2-1.0.6
  mingw make
  mingw make install PREFIX=$LIB_DIR/usr/local
fi

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
  sudo mv libpython27.dll.a /usr/i686-w64-mingw32/lib/libpython27.a
fi

# Compile Boost
if [ ! -d "$HOME/boost_1_57_0" ]; then
  cd $HOME
  wget http://downloads.sourceforge.net/project/boost/boost/1.57.0/boost_1_57_0.tar.gz
  tar xzf boost_1_57_0.tar.gz
  cd boost_1_57_0
  # We set Python version manually, so disable in that step to prevent an entry in project-config.jam
  ./bootstrap.sh -without-libraries=python
  # Set cross compiler and custom python path
  echo "using gcc : 4.8 : i686-w64-mingw32-g++ ;" > user-config.jam
  echo "using python : 2.7 : /usr/bin/python : $HOME/.wine/drive_c/Python27/include : $LIB_DIR/usr/local/lib ;" >> user-config.jam
  ./b2 -q --user-config=user-config.jam toolset=gcc target-os=windows threading=multi threadapi=win32 variant=release link=static runtime-link=static --without-context --without-coroutine install --prefix=$LIB_DIR/usr/local include=$LIB_DIR/usr/local/include
fi

# Compile core
if [ ! -d "$HOME/dlvhex-core" ]; then
  cd $HOME
  git clone --recursive git://github.com/hexhex/core.git dlvhex-core
  cd dlvhex-core
  ./bootstrap.sh

  # TODO: Check if really needed and if set from bash script
  #export PYTHON_BIN=python27
  #export PYTHON_INCLUDE_DIR=$HOME/.wine/drive_c/Python27/include

  # Patch the gringo patch for cross-compiling
  sed -i "1s|^|62c62\n< env['CXX']            = 'g++'\n---\n> env['CXX']            = 'i686-w64-mingw32-g++'\n68c68\n< env['LIBPATH']        = []\n---\n> env['LIBPATH']        = ['$LIB_DIR/usr/local/lib', '$MINGW_DIR/libs']\n|" buildclaspgringo/SConstruct.patch

  # see http://curl.haxx.se/docs/faq.html#Link_errors_when_building_libcur about CURL_STATICLIB define
  mingw ./configure LDFLAGS="-static -static-libgcc -static-libstdc++ -L$LIB_DIR/usr/local/lib" --prefix $LIB_DIR/usr/local PKG_CONFIG_PATH=$LIB_DIR/usr/local/lib/pkgconfig --enable-python --enable-static --disable-shared --enable-static-boost --with-boost=$LIB_DIR/usr/local --with-libcurl=$LIB_DIR/usr/local --host=i686-w64-mingw32 CFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB" CXXFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB" CPPFLAGS="-static -DBOOST_PYTHON_STATIC_LIB -DCURL_STATICLIB"
  mingw make
  mingw make install PREFIX=$LIB_DIR/usr/local
fi
