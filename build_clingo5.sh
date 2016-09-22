if test -e clingo5; then
	if test -d clingo5; then
		echo "clingo5 directory exists, assuming we have a configured clingo5"
		echo "(remove the directory and restart make if this not the case)"
	else
		echo "clasp exists but is no directory!";
		exit -1;
	fi
else
	echo "unpacking clingo5"
	unzip ../clingo5.zip
	mv clingo-master clingo5

	patch clingo5/SConstruct <$TOP_SRCDIR/buildclingo5/SConstruct.patch ||
		{ echo "gringo patching failed!"; exit -1; }
	patch clingo5/SConstruct <$TOP_SRCDIR/buildclingo5/SConstruct-lua.patch ||
		{ echo "gringo patching failed!"; exit -1; }
	patch clingo5/SConstruct <$TOP_SRCDIR/buildclingo5/SConstruct-python.patch ||
		{ echo "gringo patching failed!"; exit -1; }
	patch clingo5/libgringo/src/python.cc <$TOP_SRCDIR/buildclingo5/python.cc.patch ||
		{ echo "gringo patching failed!"; exit -1; }

	echo "configuring clingo5"
	mkdir -p clingo5/build/fpic
	pushd clingo5/build
	ln -s fpic release
	popd
	(
		cd clingo5
		scons --build-dir=release
	)
fi

