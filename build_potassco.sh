
if test -e clasp; then
	if test -d clasp; then
		echo "clasp directory exists, assuming we have a configured clasp"
		echo "(remove the directory and restart make if this not the case)"
	else
		echo "clasp exists but is no directory!";
		exit -1;
	fi
else
	echo "unpacking clasp"
	tar xzf ${TOP_SRCDIR}/clasp-2.1.1-source.tar.gz #--transform 's/clasp-2.1.1/clasp/'
	mv clasp-2.1.1 clasp

	echo "configuring clasp"
	mkdir -p clasp/build/fpic
	pushd clasp/build
	ln -s fpic release
	popd
	(
		cd clasp
		./configure.sh --config=fpic CXX="$CXX" CXXFLAGS=-fPIC ||
			{ echo "configuring clasp failed!"; exit -1; }
	)
fi

if test -e gringo; then
	if test -d gringo; then
		echo "gringo directory exists, assuming we have a configured gringo"
		echo "(remove the directory and restart make if this not the case)"
	else
		echo "gringo exists but is no directory!";
		exit -1;
	fi
else
	echo "unpacking gringo"
	tar xzf ${TOP_SRCDIR}/gringo-3.0.4-source.tar.gz #--transform 's/gringo-3.0.4-source/gringo/'
	mv gringo-3.0.4-source gringo

	echo "patching gringo"
	patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo.patch ||
		{ echo "gringo patching failed!"; exit -1; }
	patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-patch-cond.diff ||
		{ echo "gringo patching failed!"; exit -1; }
	patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-patch-domain-fwd-decl.diff ||
		{ echo "gringo patching failed!"; exit -1; }
	patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-patch-domain-fwd-decl-builtsource.diff ||
		{ echo "gringo patching failed!"; exit -1; }
	patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-patch-unpool-pred.diff ||
		{ echo "gringo patching failed!"; exit -1; }

	echo "using clang: ${USING_CLANG}"
	if test "xyes" == "x${USING_CLANG}"; then
		echo "patching gringo (for clang)"
		patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-clang.patch ||
			{ echo "gringo patching failed!"; exit -1; }
	fi

	mkdir -p gringo/build/release
fi

echo "making clasp"
make -C clasp/build/release VERBOSE=1 ||
	{ echo "building clasp failed!"; exit -1; }


echo "making gringo"
(
  cd gringo/build/release
  CXX=$CXX \
  cmake ../../ -DCMAKE_CXX_FLAGS="-Wall -fPIC" -DCMAKE_BUILD_TYPE=release -DWITH_LUA=none ||
    { echo "gringo cmake failed!"; exit -1; }
  make gringo-app clingo-app VERBOSE=1 ||
    { echo "gringo make failed!"; exit -1; }
)

# now it should also be possible to do the following:
#./configure --with-libclasp=`pwd`/clasp/ --with-libgringo=`pwd`/gringo/

