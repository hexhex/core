
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
	tar xzf ${TOP_SRCDIR}/clasp-3.0.1-source.tar.gz
	mv clasp-3.0.1 clasp

	echo "configuring clasp"
	mkdir -p clasp/build/fpic
	pushd clasp/build
	ln -s fpic release
	popd
	(
		cd clasp
		./configure.sh --config=fpic CXX="$CXX -DNDEBUG -O3" CXXFLAGS=-fPIC ||
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
	tar xzf ${TOP_SRCDIR}/gringo-4.3.0-source.tar.gz
	mv gringo-4.3.0-source gringo
	pushd $TOP_SRCDIR/buildclaspgringo
	patch gringo/SConstruct <SConstruct.patch ||
		{ echo "gringo patching failed!"; exit -1; }
	popd
	pushd $TOP_SRCDIR/buildclaspgringo
	patch gringo/app/gringo/main.cc <main.cc.patch ||
		{ echo "gringo patching failed!"; exit -1; }
	popd
	mkdir -p gringo/build/release
fi

echo "making clasp"
make -C clasp/build/release VERBOSE=1 ||
	{ echo "building clasp failed!"; exit -1; }


echo "making gringo"
(
  cd gringo
  CXX=$CXX \
  BOOST_ROOT=$BOOST_ROOT scons --build-dir=release gringo ||
    { echo "gringo cmake failed!"; exit -1; }
)

