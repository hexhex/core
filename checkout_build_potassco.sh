POTASSCO_REPOROOT=https://potassco.svn.sourceforge.net/svnroot/potassco/

if test -e clasp; then
	if test -d clasp; then
		echo "clasp directory exists, assuming we have a configured clasp (remove dir if this not the case)"
	else
		echo "clasp exists but is no directory!";
		exit -1;
	fi
else
	echo "checking out clasp"
  svn co $POTASSCO_REPOROOT/tags/clasp-2.0.5 clasp
	mkdir -p clasp/build/release
	echo "configuring clasp"
	pushd clasp
		./configure.sh || { echo "configuring clasp failed!"; exit -1; }
	popd
fi

echo "making clasp"
pushd clasp/build/release
  make VERBOSE=1 || { echo "building clasp failed!"; exit -1; }
popd

if test -e gringo; then
	if test -d gringo; then
		echo "gringo directory exists, assuming we have a configured gringo (remove dir if this is not the case)"
	else
		echo "gringo exists but is no directory!";
		exit -1;
	fi
else
	echo "checking out gringo";
  svn co $POTASSCO_REPOROOT/tags/gringo-3.0.4 gringo
  mkdir -p gringo/build/release
  echo "patching gringo (for multithreaded)"
  patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo.patch ||
    { echo "gringo patching failed!"; exit -1; }
  #echo "patching gringo (for clang)"
  #patch -d gringo -p0 <gringo/patches/patch-clang.diff ||
  #  { echo "gringo patching failed!"; exit -1; }
fi

echo "making gringo"
pushd gringo/build/release
  cmake ../../ -DCMAKE_CXX_FLAGS=-Wall -DCMAKE_BUILD_TYPE=release ||
    { echo "gringo cmake failed!"; exit -1; }
  make gringo-app clingo-app VERBOSE=1 ||
    { echo "gringo make failed!"; exit -1; }
popd

#./configure --with-libclasp=`pwd`/clasp/ --with-libgringo=`pwd`/gringo/

