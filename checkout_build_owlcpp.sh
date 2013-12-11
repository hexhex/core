# $1: $(OWLCPP_ROOT): If equivalept to $OWLCPPMAINDIR, owlcpp is actually downloaded and built. Otherwise we create only symbolic links to the libraries.
# $2: Boost version to use

if [ $# -eq 0 ]; then
	echo "Error: \$1 must point to OWLCPP_ROOT"
	exit 1
fi

OWLCPPMAINDIR=`cd ./$TOP_SRCDIR/owlcpp; pwd`
OWLCPP_ROOT=$1

echo "Building owlcpp in $OWLCPPMAINDIR"
if test $OWLCPPMAINDIR == $OWLCPP_ROOT; then

	LIBXML2V=2.9.0
	RAPTOR2V=2.0.8
	FACTPPV=1.6.2
	OWLCPPV=0.3.3
	if [ $# -gt 1 ]; then
		boost_major_version=$2
		BOOSTV="${boost_major_version:0:1}.${boost_major_version:1:2}.0"
		if [ $boost_major_version -lt 155 ]; then
			echo -e "Warning: dlvhex was built with boost version $BOOSTV, but owlcpp needs at least 1.55.0.\nWill build owlcpp with boost 1.55.0, but it might be incompatible with dlvhex!"
			BOOSTV="1.55.0"
		fi
	else
		# default
		BOOSTV=1.55.0
	fi

	if [ ! -f $OWLCPPMAINDIR/owlcpp-v$OWLCPPV.zip ]
	then
		echo "Downloading owlcpp source version $OWLCPPV"
		wget -O $OWLCPPMAINDIR/owlcpp-v$OWLCPPV.zip http://downloads.sourceforge.net/project/owl-cpp/v0.3.3/owlcpp-v$OWLCPPV.zip
		if [ $? -gt 0 ]
		then
			echo "Error while downloading owlcpp; aborting"
			exit 1
		fi
	fi

	BOOSTVU=$(echo $BOOSTV | sed 's/\./_/g')
	if [ ! -f $OWLCPPMAINDIR/boost-$BOOSTV.tar.gz ]
	then
		echo "Downloading boost source version $BOOSTV"
		wget -O $OWLCPPMAINDIR/boost-$BOOSTV.tar.gz http://downloads.sourceforge.net/project/boost/boost/$BOOSTV/boost_$BOOSTVU.tar.gz
		if [ $? -gt 0 ]
		then
			echo "Error while downloading boost; aborting"
			exit 1
		fi
	fi

	if [ ! -f $OWLCPPMAINDIR/libxml2-$LIBXML2V.zip ]
	then
		echo "Downloading libxml2 source version $LIBXML2V"
		wget -O $OWLCPPMAINDIR/libxml2-$LIBXML2V.zip https://git.gnome.org/browse/libxml2/snapshot/libxml2-$LIBXML2V.zip
		if [ $? -gt 0 ]
		then
			echo "Error while downloading libxml2; aborting"
			exit 1
		fi
	fi

	if [ ! -f $OWLCPPMAINDIR/raptor2-$RAPTOR2V.tar.gz ]
	then
		echo "Downloading raptor2 source version $RAPTOR2V"
		wget -O $OWLCPPMAINDIR/raptor2-$RAPTOR2V.tar.gz http://download.librdf.org/source/raptor2-$RAPTOR2V.tar.gz
		if [ $? -gt 0 ]
		then
			echo "Error while downloading raptor2; aborting"
			exit 1
		fi
	fi

	if [ ! -f $OWLCPPMAINDIR/FaCTpp-src-v$FACTPPV.tar.gz ]
	then
		echo "Downloading FaCT++ source version $FACTPPV"
		wget -O $OWLCPPMAINDIR/FaCTpp-src-v$FACTPPV.tar.gz http://factplusplus.googlecode.com/files/FaCTpp-src-v$FACTPPV.tgz
		if [ $? -gt 0 ]
		then
			echo "Error while downloading FaCT++; aborting"
			exit 1
		fi
	fi

	echo "Extracting archives"
	if [ ! -d $OWLCPPMAINDIR/owlcpp-v$OWLCPPV ]; then
		unzip $OWLCPPMAINDIR/owlcpp-v$OWLCPPV.zip
	fi
	OWLCPP_ROOT=$OWLCPPMAINDIR/owlcpp-v$OWLCPPV
	if [ ! -d $OWLCPPMAINDIR/boost_$BOOSTVU ]; then
		tar -xzf $OWLCPPMAINDIR/boost-$BOOSTV.tar.gz
	fi
	if [ ! -d $OWLCPPMAINDIR/libxml2-$LIBXML2V ]; then
		unzip $OWLCPPMAINDIR/libxml2-$LIBXML2V.zip
	fi
	if [ ! -d $OWLCPPMAINDIR/raptor2-$RAPTOR2V ]; then
		tar -xzf $OWLCPPMAINDIR/raptor2-$RAPTOR2V.tar.gz
	fi
	if [ ! -d $OWLCPPMAINDIR/FaCT++-$FACTPPV ]; then
		tar -xzf $OWLCPPMAINDIR/FaCTpp-src-v$FACTPPV.tar.gz
	fi

	echo "Generating user-config.jam"
	cd owlcpp-v$OWLCPPV
	cp doc/user-config.jam user-config.jam
	echo " constant BOOST : \"$OWLCPPMAINDIR/boost_$BOOSTVU/\" $BOOSTV ;" >> user-config.jam
	echo " constant LIBXML2 : \"$OWLCPPMAINDIR/libxml2-$LIBXML2V\" $LIBXML2V ;" >> user-config.jam
	echo " constant RAPTOR : \"$OWLCPPMAINDIR/raptor2-$RAPTOR2V\" $RAPTOR2V ;" >> user-config.jam
	echo " constant FACTPP : \"$OWLCPPMAINDIR/FaCT++-$FACTPPV\" $FACTPPV ;" >> user-config.jam
	cd ..

	if [ ! -f $OWLCPPMAINDIR/boost_$BOOSTVU/tools/build/v2/b2 ]; then
		echo "Building boost.build"
		pushd $OWLCPPMAINDIR/boost_$BOOSTVU/tools/build/v2 > /dev/null
		./bootstrap.sh > $OWLCPPMAINDIR/output.out 2>&1
		if [ $? -gt 0 ]; then
			cat $OWLCPPMAINDIR/output.out
			echo "Building boost.build failed; aborting"
			popd > /dev/null
			exit 1
		fi
		popd > /dev/null
		cp $OWLCPPMAINDIR/boost_$BOOSTVU/tools/build/v2/b2 $OWLCPPMAINDIR/owlcpp-v$OWLCPPV/
	fi

	if [ ! -f $OWLCPPMAINDIR/boost_$BOOSTVU/b2 ]; then
		echo "Building boost"
		pushd $OWLCPPMAINDIR/boost_$BOOSTVU > /dev/null
		./bootstrap.sh > $OWLCPPMAINDIR/output.out 2>&1
		if [ $? -gt 0 ]; then
			cat $OWLCPPMAINDIR/output.out
			echo "Building boost failed; aborting"
			popd > /dev/null
			exit 1
		fi
		./b2 "$params" > $OWLCPPMAINDIR/output.out 2>&1
		if [ $? -gt 0 ]; then
			cat $OWLCPPMAINDIR/output.out
			echo "Building boost failed; aborting"
			cd popd > /dev/null
			exit 1
		fi
		popd > /dev/null
	fi

	if [ ! -f $OWLCPPMAINDIR/libxml2-$LIBXML2V/include/libxml/xmlversion.h ]; then
		echo "Building libxml2"
		pushd $OWLCPPMAINDIR/libxml2-$LIBXML2V > /dev/null
		./autogen.sh > $OWLCPPMAINDIR/output.out 2>&1
		if [ $? -gt 0 ]; then
			cat $OWLCPPMAINDIR/output.out
			echo "Building libxml2 failed; aborting"
			popd > /dev/null
			exit 1
		fi
		popd > /dev/null
	fi

	#if [ ! -f $OWLCPPMAINDIR/raptor2-$RAPTOR2V/src/raptor_nfc_data.c ]; then
	#	echo "Building raptor2"
	#	pushd $OWLCPPMAINDIR/raptor2-$RAPTOR2V > /dev/null
	#	./autogen.sh
	#	if [ $? -gt 0 ]; then
	#		echo "Building raptor2 failed; aborting"
	#		popd > /dev/null
	#		exit 1
	#	fi
	#	popd > /dev/null
	#fi

	echo "Building owlcpp"
	REALHOME=$HOME
	export HOME=$OWLCPPMAINDIR/owlcpp-v$OWLCPPV
	export BOOST_ROOT=$OWLCPPMAINDIR/boost_$BOOSTVU
	export BOOST_BUILD_PATH=$OWLCPPMAINDIR/boost_$BOOSTVU/tools/build/v2
	pushd $OWLCPPMAINDIR/owlcpp-v$OWLCPPV > /dev/null
	$BOOST_ROOT/tools/build/v2/b2 release "$params" > $OWLCPPMAINDIR/output.out 2>&1
	ret=$?
	export HOME=$REALHOME
	if [ $ret -gt 0 ]; then
		cat $OWLCPPMAINDIR/output.out
		echo "Building owlcpp failed; aborting"
		exit 1
	fi
fi

# scan $(OWLCPP_ROOT) for .a and header files and create symbolic links in $OWLCPPMAINDIR/include and $OWLCPPMAINDIR/libs
echo "Creating symbolic links to owlcpp"
rm $OWLCPPMAINDIR/include
ln -s $OWLCPP_ROOT/include $OWLCPPMAINDIR/include
mkdir $OWLCPPMAINDIR/libs 2> /dev/null
rm $OWLCPPMAINDIR/libs/*.a
ln -s $(ls $OWLCPP_ROOT/out/bin/io/*/release/link-static/libowlcpp_io.a | head) $OWLCPPMAINDIR/libs/libowlcpp_io.a
ln -s $(ls $OWLCPP_ROOT/out/bin/logic/*/release/link-static/libowlcpp_logic.a | head) $OWLCPPMAINDIR/libs/libowlcpp_logic.a
ln -s $(ls $OWLCPP_ROOT/out/bin/rdf/*/release/link-static/libowlcpp_rdf.a | head) $OWLCPPMAINDIR/libs/libowlcpp_rdf.a

