# note: this method is deprecated, we now download the tarballs in
# bootstrap.sh and use build_potassco.sh (saves time and bandwidth)

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
  svn co $POTASSCO_REPOROOT/tags/clasp-3.0.5 clasp
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
  svn co $POTASSCO_REPOROOT/tags/gringo-4.3.0 gringo
  mkdir -p gringo/build/release
fi

echo "making gringo"
pushd gringo/build/release
  cmake ../../ -DCMAKE_CXX_FLAGS=-Wall -DCMAKE_BUILD_TYPE=release -DWITH_LUA=none ||
    { echo "gringo cmake failed!"; exit -1; }
popd

