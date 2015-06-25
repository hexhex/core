# install dependencies only (except boost):
# sudo apt-get install -y git gcc-4.8 g++-4.8 g++ bison re2c scons cmake automake autoconf libstdc++-4.8-dev libcurl4-openssl-dev libbz2-dev python2.7-dev libpython-all-dev libtool

# Make sure only root can run our script
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

# prepare installation of dlvhex
mkdir dlvhex >/dev/null 2>&1
cd dlvhex

# 1. install dependencies:
read -p "Do you want to install the dependencies? (y/n) " depyn
case $depyn in
[Yy]* )
	apt-get install -y git gcc-4.8 g++-4.8 g++ bison re2c scons cmake automake autoconf libstdc++-4.8-dev libcurl4-openssl-dev libbz2-dev python2.7-dev libpython-all-dev libtool;
	break;;
[Nn]* )
	break;;
* ) echo "Please answer yes or no.";;
esac

# 2. install boost
dlvhexoptions=""
read -p "Does your Linux distribution contain libboost in version >=1.55? If you type no, it will be installed locally. (y/n) " yn
case $yn in
[Yy]* )
	case $depyn in
	[Yy]* )
		# 2a. if boost version in the distribution is >=1.55
		apt-get install -y libboost-all-dev;
		break;;
	* ) ;;
	esac
	break;;
[Nn]* )
	# 2b. otherwise: otherwise install manually
	read -p "Please enter the path where you wish to install boost: " boostdir
	wget -O boost_1_55_0.tar.gz http://ufpr.dl.sourceforge.net/project/boost/boost/1.55.0/boost_1_55_0.tar.gz;
	tar xvf boost_1_55_0.tar.gz;
	cd boost_1_55_0 && ./bootstrap.sh && ./b2 --python-enable --layout=tagged --build-type=complete --threading=multi --prefix=$boostdir install -j4;
	dlvhexoptions="$dlvhexoptions --with-boost=$boostdir";
	break;;
* ) echo "Please answer yes or no.";;
esac

# 3. make dlvhex:
read -p "Do you want to install dlvhex globally? (y/n) " yn
case $yn in
[Yy]* )
	break;;
[Nn]* )
	read -p "Please enter path where you wish to install dlvhex: " dlvhexpath;
	dlvhexoption="$dlvhexoption --prefix=$dlvhexpath";
	break;;
* ) echo "Please answer yes or no.";;
esac
read -p "Do you want to build dlvhex in debug mode? (y/n) " yn
case $yn in
[Yy]* )
	dlvhexoption="$dlvhexoption --enable-debug";
	break;;
[Nn]* )
	dlvhexoption="$dlvhexoption --enable-release";
	break;;
* ) echo "Please answer yes or no.";;
esac

git clone https://github.com/hexhex/core --recursive
export PYTHON_BIN=python2.7 && cd core && ./bootstrap.sh && ./configure --enable-python $dlvhexoptions && make -j4 && make install

