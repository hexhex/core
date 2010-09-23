#!/bin/bash
CC=clang CXX=clang++ \
LDFLAGS="${LDFLAGS} -L`pwd`/../dlv-lib/" \
CPPFLAGS="${CPPFLAGS} -I`pwd`/../dlv-lib/" \
LD_LIBRARY_PATH="`pwd`/../dlv-lib/:${LD_LIBRARY_PATH}" \
../configure --with-boost=/home/staff/ps/include/boost-release/
