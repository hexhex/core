
POTASSCO_REPOROOT=https://potassco.svn.sourceforge.net/svnroot/potassco/
TOP_SRCDIR=.

GRINGOFNAME=gringo-3.0.4-source.tar.gz
if [ ! -e $GRINGOFNAME ]; then
  echo "downloading $GRINGOFNAME"
  wget http://downloads.sourceforge.net/project/potassco/gringo/3.0.4/$GRINGOFNAME
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
	patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-patch-cond.patch ||
		{ echo "gringo patching failed!"; exit -1; }
	patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-patch-domain-fwd-decl.patch ||
		{ echo "gringo patching failed!"; exit -1; }
	patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-patch-domain-fwd-decl-builtsource.patch ||
		{ echo "gringo patching failed!"; exit -1; }
	patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-patch-unpool-pred.patch ||
		{ echo "gringo patching failed!"; exit -1; }

# Edit: it seems that this patch is also necessary with gcc if boost version >= 1.49
#	echo "using clang: ${USING_CLANG}"
#	if test "xyes" == "x${USING_CLANG}"; then
#		echo "patching gringo (for clang)"
		patch -d gringo -p0 <$TOP_SRCDIR/buildclaspgringo/gringo-clang.patch ||
			{ echo "gringo patching failed!"; exit -1; }
#	fi

	mkdir -p gringo/build/release
fi
