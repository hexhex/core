AC_PREREQ([2.61])
AC_INIT([dlvhex2],[2.0.0],[dlvhex-devel@lists.sourceforge.net])
AM_INIT_AUTOMAKE([-Wall gnu check-news 1.10])

AC_CONFIG_SRCDIR([src/dlvhex.cpp])
AC_CONFIG_HEADER([include/config.h])

# provide package and ABI version to substitute in .in files and to config.h
DLVHEX_DEFINE_VERSION([DLVHEX],[$PACKAGE_VERSION])
# for ABI and library versioning,
# see https://sourceforge.net/apps/trac/dlvhex/wiki/LibraryVersions
DLVHEX_DEFINE_VERSION([DLVHEX_ABI],[7.0.0])

AC_CANONICAL_HOST

#
# check host system
# this will be used in include/common.h.in
#
case $host in
    *-*-msdos* | *-*-go32* | *-*-mingw32* | *-*-cygwin* | *-*-windows*)
        HAVE_WIN32=1
        ;;
    *)
        ;;
esac
AC_SUBST([HAVE_WIN32])

# Check for C/C++ compiler (reset {C,CXX}FLAGS, otw. autoconf uses -g -O2 by default)
save_CFLAGS="$CFLAGS"
save_CXXFLAGS="$CXXFLAGS"
${CFLAGS=""}
${CXXFLAGS=""}
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXXCPP

CFLAGS="$CFLAGS $save_CFLAGS"
CXXFLAGS="$CXXFLAGS $save_CFLAGS"


# check compiler

# set default warning parameters for g++ compatible compilers
# allow to disable certain warnings using custom CXXFLAGS
if test "$GXX" = "yes"; then
   CXXFLAGS="-W -Wall $CXXFLAGS"

   AC_MSG_CHECKING([for g++ >= 4.3 or clang++ >= 2.9])
   AC_LANG_PUSH([C++])
   AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
 int dummy;
#else
# if __clang__ == 1
#  if __clang_major__ >= 3 || (__clang_major__ == 2 && __clang_minor__ >= 9)
 int dummy;
#  else
#    error clang is too old, we need at least clang 2.9
#  endif
# else
#  error gcc is too old, we need at least gcc 4.3
# endif
#endif 
]], [[
dummy;
]])],
[
	AC_MSG_RESULT([yes])
],
[
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([Cannot find g++ >= 4.3 or clang++ >= 2.9. Please upgrade your compiler.])
]
)
AC_LANG_POP()

fi


# check for clang
case $CXX in
   clang*)
     CXXFLAGS="$CXXFLAGS -ferror-limit=2"
     # TODO check if boost >= 1.46, clang only works with this
   ;;
   *)
   ;;
esac

AC_CACHE_SAVE

# Checks for header files.
AC_HEADER_DIRENT
AC_CHECK_HEADERS([sys/param.h])

# by default we look into /opt/local and /sw for include files (macports vs. fink)
case $host_os in
     darwin*|macosx*)
	CPPFLAGS="$CPPFLAGS -I/opt/local/include -I/sw/include"
     ;;
esac 


#
# debug and release flags
#
# these must be processed early, as they influence,
# e.g., the boost library selection (_GLIBCXX_DEBUG requires the -d library types)

#
# --enable-debug vs --enable-release vs --enable-benchmark vs --enable-profiling vs --enable-extradebug
# (only one of the three can be chosen, default is --enable-release)
#
AC_ARG_ENABLE(debug,
              [AS_HELP_STRING([--enable-debug],[build debug version (default)])],
              [enable_debug=true],
              [enable_debug=false]
             )
AC_ARG_ENABLE(release,
              [AS_HELP_STRING([--enable-release],[build release version (-assertions,-debuglogging)])],
              [enable_release=true],
              [enable_release=false]
             )
AC_ARG_ENABLE(benchmark,
              [AS_HELP_STRING([--enable-benchmark],[build benchmark version (release+timers+counters)])],
              [enable_benchmark=true],
              [enable_benchmark=false]
             )
AC_ARG_ENABLE(profiling,
              [AS_HELP_STRING([--enable-profiling],[build profiling version (+release+profiling)])],
              [enable_profiling=true],
              [enable_profiling=false]
             )
AC_ARG_ENABLE(extradebug,
              [AS_HELP_STRING([--enable-extradebug],[switch on GLIBCXX debugging switches (e.g., safe iterators) (requires boost library built with the same flags)])],
              [enable_extradebug=true],
              [enable_extradebug=false]
              )

# default = release
dbgrelbench="$enable_debug,$enable_release,$enable_benchmark,$enable_profiling,$enable_extradebug"
if test "$dbgrelbench" = "false,false,false,false,false"; then
  enable_release=true
  dbgrelbench="$enable_debug,$enable_release,$enable_benchmark,$enable_profiling,$enable_extradebug"
fi


optlevel=""
debuginfo=""
prof=""
defines=""

# process selection
case "$dbgrelbench" in

     true,*,*,*,*)
	# do stuff that must be done for --enable-debug here
	AC_DEFINE(DEBUG, [], [Activates assertions and debug logging])
  CPPFLAGS="$CPPFLAGS -DDEBUG"
	optlevel="-O0"
	debuginfo="-g"
	enable_release=false
	enable_benchmark=false
	enable_profiling=false
  AC_SUBST([DISTCHECK_CONFIGURE_FLAGS],
           ["$DISTCHECK_CONFIGURE_FLAGS --enable-debug"])
     ;;

     *,*,*,*,true)
	# do stuff that must be done for --enable-extra-debug here
	AC_DEFINE(DEBUG, [], [Activates assertions and debug logging])
  CPPFLAGS="$CPPFLAGS -DDEBUG"
	optlevel="-O0"
	defines="-D_GLIBCXX_DEBUG"
	#defines="-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC"
	debuginfo="-g"
	enable_release=false
	enable_benchmark=false
	enable_profiling=false
     ;;

     *,*,true,*,*)
	# do stuff that must be done for --enable-benchmark here
	# (we have an implicit DLVHEX_BENCHMARK definition in config.h)
	AC_DEFINE(DLVHEX_BENCHMARK, [], [Turns on benchmarking timers and counters])
	AC_DEFINE(NDEBUG, [], [Turns off debug logging and assertions])
  CPPFLAGS="$CPPFLAGS -DNDEBUG"
	optlevel="-O3"
  AC_SUBST([DISTCHECK_CONFIGURE_FLAGS],
           ["$DISTCHECK_CONFIGURE_FLAGS --enable-benchmark"])
     ;&

     *,*,*,true,*)
	# do stuff that must be done for --enable-profiling here
	AC_DEFINE(NDEBUG, [], [Turns off debug logging and assertions])
  CPPFLAGS="$CPPFLAGS -DNDEBUG"
	optlevel="-O3"
	prof="-pg"
     ;&

     *,true,*,*,*)
	# do stuff that must be done for --enable-release here
	AC_DEFINE(NDEBUG, [], [Turns off debug logging and assertions])
  CPPFLAGS="$CPPFLAGS -DNDEBUG"
	optlevel="-O3"
  AC_SUBST([DISTCHECK_CONFIGURE_FLAGS],
           ["$DISTCHECK_CONFIGURE_FLAGS --enable-release"])
     ;&

esac

# setup CXXFLAGS and CPPFLAGS
CXXFLAGS="$CXXFLAGS $debuginfo $optlevel $prof"
CPPFLAGS="$CPPFLAGS $defines"


# libtool
LT_INIT([dlopen])


# Checks for libtool's libltdl
LT_CONFIG_LTDL_DIR([libltdl])
LTDL_INIT([installable])
#AC_SUBST(INCLTDL)
#AC_SUBST(LIBLTDL)

# configure autogenerated libltdl directory
# AC_CONFIG_SUBDIRS([libltdl])
# fix for some versions of autoconf/libtool, see also https://gnunet.org/mantis/view.php?id=1340
m4_ifdef([_AC_SEEN_TAG(libltdl)], [], [AC_CONFIG_SUBDIRS([libltdl])])

# pkg-config support
PKG_PROG_PKG_CONFIG
AC_CACHE_SAVE

# checking for boost libs
# we need 1.43 for functional/factory
BOOST_REQUIRE([1.43.0])
BOOST_SMART_PTR
BOOST_STRING_ALGO
BOOST_TOKENIZER
BOOST_IOSTREAMS
AC_CACHE_SAVE
BOOST_GRAPH
BOOST_PROGRAM_OPTIONS
AC_CACHE_SAVE
BOOST_TEST
AC_CACHE_SAVE
BOOST_SYSTEM
BOOST_FILESYSTEM
AC_CACHE_SAVE
BOOST_THREADS
# for --enable-debug, we need the boost_date_time library
if test "x$enable_debug" = "xtrue"; then
   BOOST_DATE_TIME
fi
AC_CACHE_SAVE

#
# the default system-wide plugin dir ${pkglibdir}/plugins can be
# overridden by setting SYS_PLUGIN_DIR=... at configure-time
#
if test "$SYS_PLUGIN_DIR" = ""; then
    sysplugindir="\${pkglibdir}/plugins"
else
    sysplugindir="$SYS_PLUGIN_DIR"
fi

# this goes into dlvhex.pc
AC_SUBST(sysplugindir)

# ${pkglibdir} can only be evaluated in the Makefiles, so no way to
# put it in config.h
CPPFLAGS="$CPPFLAGS -DSYS_PLUGIN_DIR='\"$sysplugindir\"'"


#
# the user plugin dir consists of the user home concatenated with
# and USER_PLUGIN_DIR, which is .dlvhex2/plugins by default and can be
# overridden by setting USER_PLUGIN_DIR=... at configure-time
#
if test "$USER_PLUGIN_DIR" = ""; then
    userplugindir=".dlvhex2/plugins"
else
    userplugindir="$USER_PLUGIN_DIR"
fi

# this goes into dlvhex.pc
AC_SUBST(userplugindir)

# this can only be evaluated in the Makefiles
CPPFLAGS="$CPPFLAGS -DUSER_PLUGIN_DIR='\"$userplugindir\"'"


WHICH_DLVHEX_TESTS="\$(top_builddir)/examples/tests/basic.test"
AC_SUBST(WHICH_DLVHEX_TESTS)

#
# third-party solver integration
# * dlv as executable
# * dlv as shared library (libdlv (nonfunctional))
# * clingo as shared library (preliminary)
#

EXTSOLVER_LIBS=
EXTSOLVER_LDFLAGS=
EXTSOLVER_CPPFLAGS=

extsolver_found=false

#
# the user didn't supply a path for dlv
#
if test "$DLV" = ""; then
    # we assume "dlv"
    DLV=dlv
    # and check if it exists
    AC_CHECK_PROG(DLVPATH, $DLV, $DLV, notfound)
else
    # the user supplied a 'DLV=...'
    # check if it exists (with search path=/, since the user supplied
    # an entire path!) and set DLVPATH to it!
    AC_CHECK_PROG(DLVPATH, $DLV, $DLV, notfound, [/])
fi

if test "$DLVPATH" = notfound; then
  AC_MSG_WARN([dlv not found. Download it at http://www.dlvsystem.com/])
else
  extsolver_found=true
  AC_DEFINE([HAVE_DLV], [1], [Defined if we found dlv.])
  AC_DEFINE_UNQUOTED(DLVPATH,"$DLVPATH",[Path of DLV executable])
fi


#
# the user didn't supply a path for dlv-lib
#
if test "$DLVLIB" = ""; then
    # we assume we don't have LIBDLV
    foo=$foo;
else
  AC_DEFINE([HAVE_LIBDLV], [1], [Defined if we found libdlv.])
  EXTSOLVER_LIBS="${EXTSOLVER_LIBS} -l${DLVLIB} -ldl -lpthread"
  extsolver_found=true
fi
# TODO and check if it exists
dnl AC_CHECK_LIB($DLVLIB, [create_program_handler], [
dnl   AC_DEFINE([HAVE_LIBDLV], [1], [Defined if we found libdlv.])
dnl   LIBS="-l${DLVLIB} $LIBS"
dnl   ], [], [-ldl -lpthread])


#
# the user didn't supply a path for dlvdb
#
if test "$DLVDB" = ""; then
    # we assume "dlvdb"
    DLVDB=dlvdb
    # and check if it exists
    AC_CHECK_PROG(DLVDBPATH, $DLVDB, $DLVDB, notfound)
else
    # the user supplied a 'DLVDB=...'
    # check if it exists (with search path=/, since the user supplied
    # an entire path!) and set DLVPATH to it!
    AC_CHECK_PROG(DLVDBPATH, $DLVDB, $DLVDB, notfound, [/])
fi

if test "$DLVDBPATH" = notfound; then
    AC_MSG_WARN([Disabled support for dlvdb. Download it at http://www.mat.unical.it/terracina/dlvdb/])
else
    # put the dlvdb path into config.h
    AC_DEFINE_UNQUOTED(DLVDBPATH,"$DLVDBPATH",[Path of DLVDB executable])
    AC_DEFINE([HAVE_DLVDB], [1], [Defined if we found dlvdb.])
  extsolver_found=true
fi

#
# libclingo (see README)
#
AC_ARG_WITH([libclingo],
  AS_HELP_STRING([--with-libclingo=<gringo-trunk-directory>],
                 [use clingo as external solver, you need to specify the directory
                  where you have built clingo-app of tag/gringo-3.0.3 without lua/luasql]),
  [with_libclingo_support=$withval],
  [with_libclingo_support=false])
# conditions in makefiles
AM_CONDITIONAL([HAVE_LIBCLINGO],[test "x${with_libclingo_support}" != xfalse])
if test "x${with_libclingo_support}" != xfalse; then
	# #define in config.h <- config.h.in
	AC_DEFINE([HAVE_LIBCLINGO], [1], [Defined if we have --with-libclingo.])
  GRINGO_TRUNK_DIR=$with_libclingo_support
  if test "x$GRINGO_TRUNK_DIR" = "xyes"; then
    AC_MSG_ERROR([You have to specify gringo trunk in --with-libclingo=<gringo-trunk-dir>])
  fi
	EXTSOLVER_LIBS="${EXTSOLVER_LIBS} -lappclingo -lclasp -lappgringo -lgringo -lprogram_opts"
	EXTSOLVER_LIBS="${EXTSOLVER_LIBS} ${BOOST_FILESYSTEM_LIBS} ${BOOST_SYSTEM_LIBS}"
	EXTSOLVER_LDFLAGS="${EXTSOLVER_LDFLAGS} -L${GRINGO_TRUNK_DIR}/build/release/lib/ ${BOOST_FILESYSTEM_LDFLAGS} ${BOOST_SYSTEM_LDFLAGS}"
  EXTSOLVER_CPPFLAGS="${EXTSOLVER_CPPFLAGS} -I${GRINGO_TRUNK_DIR}/libprogram_opts/ -I${GRINGO_TRUNK_DIR}/libgringo/ -I${GRINGO_TRUNK_DIR}/libclasp/ -I${GRINGO_TRUNK_DIR}/app/"
  WHICH_DLVHEX_TESTS="${WHICH_DLVHEX_TESTS} \$(top_builddir)/examples/tests/libclingobackend.test"
  AC_SUBST([DISTCHECK_CONFIGURE_FLAGS],
           ["$DISTCHECK_CONFIGURE_FLAGS '--with-libclingo=$GRINGO_TRUNK_DIR'"])
  extsolver_found=true
fi


#
# libclasp (see README)
#
# NOTE: It is very IMPORTANT that the libclasp section comes before the libgringo section;
#       otherwise we link against the clasp library included in gringo (which is the wrong version);
#       (ld processes search pathes in the specified order)
AC_ARG_WITH([libclasp],
  AS_HELP_STRING([--with-libclasp=<clasp-trunk-directory>],
                 [use clasp as genuine solver, you need to specify the directory
                  where you have built clasp-app of tag/clasp-2.0.5]),
  [with_libclasp_support=$withval],
  [with_libclasp_support=false])
# conditions in makefiles
AM_CONDITIONAL([HAVE_LIBCLASP],[test "x${with_libclasp_support}" != xfalse])
if test "x${with_libclasp_support}" != xfalse; then
	# #define in config.h <- config.h.in
	AC_DEFINE([HAVE_LIBCLASP], [1], [Defined if we have --with-libclasp.])
  CLASP_TRUNK_DIR=$with_libclasp_support
  if test "x$CLASP_TRUNK_DIR" = "xyes"; then
    AC_MSG_ERROR([You have to specify clasp trunk in --with-libclasp=<clasp-trunk-dir>])
  fi
	EXTSOLVER_LIBS="${EXTSOLVER_LIBS} -lclasp"
	EXTSOLVER_LIBS="${EXTSOLVER_LIBS} ${BOOST_FILESYSTEM_LIBS} ${BOOST_SYSTEM_LIBS}"
	EXTSOLVER_LDFLAGS="${EXTSOLVER_LDFLAGS} -L${CLASP_TRUNK_DIR}/build/release/libclasp/lib/ ${BOOST_FILESYSTEM_LDFLAGS} ${BOOST_SYSTEM_LDFLAGS}"
  EXTSOLVER_CPPFLAGS="${EXTSOLVER_CPPFLAGS} -I${CLASP_TRUNK_DIR}/libclasp/ -I${CLASP_TRUNK_DIR}/app/"
  extsolver_found=true
fi

#
# libgringo (see README)
#
AC_ARG_WITH([libgringo],
  AS_HELP_STRING([--with-libgringo=<gringo-trunk-directory>],
                 [use gringo as genuine grounder, you need to specify the directory
                  where you have built gringo-app of tag/gringo-3.0.3 without lua/luasql]),
  [with_libgringo_support=$withval],
  [with_libgringo_support=false])
# conditions in makefiles
AM_CONDITIONAL([HAVE_LIBGRINGO],[test "x${with_libgringo_support}" != xfalse])
if test "x${with_libgringo_support}" != xfalse; then
	# #define in config.h <- config.h.in
	AC_DEFINE([HAVE_LIBGRINGO], [1], [Defined if we have --with-libgringo.])
  GRINGO_TRUNK_DIR=$with_libgringo_support
  if test "x$GRINGO_TRUNK_DIR" = "xyes"; then
    AC_MSG_ERROR([You have to specify gringo trunk in --with-libgringo=<gringo-trunk-dir>])
  fi
	EXTSOLVER_LIBS="${EXTSOLVER_LIBS} -lappgringo -lgringo -lprogram_opts"
	EXTSOLVER_LIBS="${EXTSOLVER_LIBS} ${BOOST_FILESYSTEM_LIBS} ${BOOST_SYSTEM_LIBS}"
	EXTSOLVER_LDFLAGS="${EXTSOLVER_LDFLAGS} -L${GRINGO_TRUNK_DIR}/build/release/lib ${BOOST_FILESYSTEM_LDFLAGS} ${BOOST_SYSTEM_LDFLAGS}"
  EXTSOLVER_CPPFLAGS="${EXTSOLVER_CPPFLAGS} -I${GRINGO_TRUNK_DIR}/libprogram_opts/ -I${GRINGO_TRUNK_DIR}/libgringo/ -I${GRINGO_TRUNK_DIR}/app/"
  extsolver_found=true
fi

# TODO libclingo search for lib, test lib, set lib variables from these checks
# (at the moment we rely on the user to correctly give directory)

if test "x${extsolver_found}" = xfalse; then
  AC_MSG_ERROR([No aspsolver backend found. Please make sure at least one of dlv/dlvdb/libdlv/libclingo is available.])
fi

AC_SUBST(EXTSOLVER_CPPFLAGS)
AC_SUBST(EXTSOLVER_LDFLAGS)
AC_SUBST(EXTSOLVER_LIBS)

CPPFLAGS="$CPPFLAGS -DUSER_PLUGIN_DIR='\"$userplugindir\"'"

#
# check for libcurl
#
LIBCURL_CHECK_CONFIG([], [], [], [AC_MSG_ERROR([could not find CURL library. Download it at http://curl.haxx.se/.])])


#
# check for bmagic
#
AC_ARG_WITH(bmagic,
            [  --with-bmagic=DIR       Prefix of bitmagic library (DIR=local forces local bmagic)]
)

BM_CPPFLAGS=""
BM_PKGCONFIG_CPPFLAGS=""
BM_VERSION=""
BM_INSTALL="no"

if test x$with_bmagic != xlocal; then

   TMP_CPPFLAGS=$CPPFLAGS
   CPPFLAGS="$CPPFLAGS -I$with_bmagic"
   AC_LANG_PUSH([C++])
   AC_CHECK_HEADER([bm/bm.h], [have_bm=true], [have_bm=false])
   # todo: do some sanity compilations with this bmagic
   AC_LANG_POP([C++])
   CPPFLAGS=$TMP_CPPFLAGS

   if test x$have_bm = xfalse; then
      AC_MSG_WARN([bmagic not found. Using local bmagic.])
   else
      BM_CPPFLAGS="-I$with_bmagic"
      have_bm=true
   fi

fi

if test x$have_bm != xtrue; then
   # force local bmagic
   BM_VERSION=3.7.0
   # include bmagic for building this
   BM_CPPFLAGS="-I\${top_srcdir}/bm${BM_VERSION}/"
   # in pkgconfig we have include flags 
   # -I$(includedir)/dlvhex2/bm${BM_VERSION}/
   # as this is the location where the local bmagic headers will be installed
   # (rationale: we do not want to install over an existing bmagic if we force local bmagic)
   BM_PKGCONFIG_CPPFLAGS="-I\${includedir}/dlvhex2/bm${BM_VERSION}/"
   # this is used in SUBDIRS in makefile, to either install bitmagic headers or not
   BM_INSTALL=yes
fi

AC_SUBST(BM_INSTALL)
AC_SUBST(BM_CPPFLAGS)
AC_SUBST(BM_VERSION)
AC_SUBST(BM_PKGCONFIG_CPPFLAGS)



#
# doxygen stuff
#
# these are the defaults, can be overridden by --enable-doxygen-...
DX_HTML_FEATURE([ON])
DX_CHM_FEATURE([OFF])
DX_CHI_FEATURE([OFF])
DX_MAN_FEATURE([OFF])
DX_RTF_FEATURE([OFF])
DX_XML_FEATURE([OFF])
DX_PDF_FEATURE([OFF])
DX_PS_FEATURE([OFF])
DX_INIT_DOXYGEN([dlvhex2], [doxygen.cfg], [doc])

# output autogenerated files
AC_CONFIG_FILES([
           Makefile
           dlvhex2.pc
           examples/Makefile
           examples/tests/basic.test
           examples/tests/libclingobackend.test
           include/Makefile
           include/common.h
           include/dlvhex2/Makefile
           src/Makefile
           testsuite/Makefile
])


AC_OUTPUT

dnl output configuration
cat <<EOF

$PACKAGE_TARNAME $PACKAGE_VERSION configuration:

 - Compiler settings
 CC: $CC
 CXX: $CXX
 CFLAGS: $CFLAGS
 CPPFLAGS: $CPPFLAGS
 CXXFLAGS: $CXXFLAGS
 LDFLAGS: $LDFLAGS

 - Path settings
 prefix: $prefix
 exec_prefix: $exec_prefix

 bindir: $bindir
 libdir: $libdir
 pkglibdir: $libdir/$PACKAGE_TARNAME
 datadir: $datadir

 userplugindir: $userplugindir
 sysplugindir: $sysplugindir

 - Using the following solver backends:
 DLV: $DLVPATH
 DLVDB: $DLVDBPATH
 libclingo: $GRINGO_TRUNK_DIR
 libdlv: $DLVLIB

 - Using the following libraries:
 Bitmagic: $BM_VERSION $BM_CPPFLAGS
 Boost: $boost_major_version $BOOST_CPPFLAGS
 Curl: $libcurl_cv_lib_curl_version $LIBCURL_CPPFLAGS

 - Doxygen support:
 doxygen: $DX_DOXYGEN

 - Variant:
 Release: $enable_release
 Profiling: $enable_profiling
 Debug: $enable_debug
 Extra-Debug: $enable_extradebug

EOF
