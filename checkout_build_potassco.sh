set -x
DIR=`pwd`
#BOOST_ROOT=
#LDFLAGS="-L${BOOST_ROOT}/lib/ ${LDFLAGS}"
pushd ${DIR}
#test -e clasp && { echo "directory clasp/ already exists! please remove!"; exit -1; }
#test -e gringo && { echo "directory gringo/ already exists! please remove!"; exit -1; }

#svn co https://potassco.svn.sourceforge.net/svnroot/potassco/tags/clasp-2.0.5 clasp
mkdir -p ${DIR}/clasp/build/release
cd ${DIR}/clasp/
#./configure.sh
cd ${DIR}/clasp/build/release
make VERBOSE=1 || { echo "building clasp failed!"; exit -1; }

#svn co https://potassco.svn.sourceforge.net/svnroot/potassco/tags/gringo-3.0.4 gringo
mkdir -p ${DIR}/gringo/build/release
sed -i 's/^set(Boost_USE_MULTITHREADED .*)$/set(Boost_USE_    MULTITHREADED ON)/' ${DIR}/gringo/CMakeLists.txt
cd ${DIR}/gringo/build/release
cmake ../../ -DCMAKE_CXX_FLAGS=-Wall -DCMAKE_BUILD_TYPE=release
make gringo-app clingo-app VERBOSE=1

popd
set +x

echo "you can now configure dlvhex as follows: ./configure --with-libclasp=${DIR}/clasp/ --with-libgringo=${DIR}/gringo/"

