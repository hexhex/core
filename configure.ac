
AC_PREREQ([2.59])
AC_INIT([dlvhex],[1.4.0],[roman@kr.tuwien.ac.at])
AC_CONFIG_SRCDIR([src/dlvhex/dlvhex.cpp])
AC_CONFIG_HEADER([src/config.h])

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

#
# the default system-wide plugin dir $(pkglibdir)/plugins can be
# overridden by setting PLUGIN_DIR=... at configure-time
#
if test "$SYS_PLUGIN_DIR" = ""; then
    sysplugindir="\${pkglibdir}/plugins"
else
    sysplugindir="$SYS_PLUGIN_DIR"
fi

AC_SUBST(sysplugindir)

CPPFLAGS="$CPPFLAGS -DSYS_PLUGIN_DIR='\"$sysplugindir\"'"

#
# the user plugin dir consists of the user home concatenated with
# and USER_PLUGIN_DIR, which is .dlvhex/plugins by default and can be
# overridden by setting USER_PLUGIN_DIR=... at configure-time
#
if test "$USER_PLUGIN_DIR" = ""; then
    userplugindir=".dlvhex/plugins"
else
    userplugindir="$USER_PLUGIN_DIR"
fi

AC_SUBST(userplugindir)

CPPFLAGS="$CPPFLAGS -DUSER_PLUGIN_DIR='\"$userplugindir\"'"

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
    AC_MSG_ERROR([dlv not found. Download it at http://www.dlvsystem.com])
fi

#
# put the dlv path into config.h
#
AC_DEFINE_UNQUOTED(DLVPATH,"$DLVPATH",[Path of DLV executable])


AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CC
AM_PROG_LEX
# include bison generated-stuff in dist
AC_PROG_YACC


# Checks for libraries.

# Checks for header files.
AC_HEADER_DIRENT
AC_CHECK_HEADERS([sys/param.h])

#
# search for FlexLexer.h so we can add it at `make dist' time
#
absolute_srcdir=`pwd`
absolute_srcdir=$absolute_srcdir/$srcdir
FLEXLEXER_H=$absolute_srcdir/src/dlvhex/FlexLexer.h
if test -r $FLEXLEXER_H ; then : ; else

	AC_LANG_PUSH([C++])
	AC_CHECK_HEADER([FlexLexer.h])
	AC_LANG_POP([C++])

	# first we guess the prefix of flex
	AC_PATH_PROG([flexexec], [flex], [notfound])
	flexbindir=`AS_DIRNAME(["$flexexec"])`
	flexprefix=`AS_DIRNAME(["$flexbindir"])`

	if test "$flexexec" = notfound; then
	   AC_MSG_ERROR([flex not found. Download it at http://flex.sourceforge.net/])
	fi

	include_paths="$flexprefix/include"

	# then we check all compiler flags for include paths
	flags="$CPPFLAGS $CXXFLAGS $CFLAGS"
	for f in $flags
	do
		case $f in
		-I*) include_paths="$include_paths $f" ;;
		esac
	done

	# TODO: get default include paths from compiler?
	include_paths="$include_paths /usr/local/include /usr/include"

	# get rid of -I compiler flags
	paths=`echo $include_paths | sed s,-I,,g`

	AC_MSG_CHECKING([for FlexLexer.h path in $paths])

	# get the FIRST occurence of FlexLexer.h
	FLEXLEXER_H=`find $paths -maxdepth 1 -name FlexLexer.h 2>/dev/null | head -n1`

	AC_MSG_RESULT([$FLEXLEXER_H])

	if test -z "$FLEXLEXER_H" ; then AC_MSG_ERROR([Could not find FlexLexer.h]) fi

fi

AC_SUBST([FLEXLEXER_H])



# boost stuff
AC_LANG_PUSH([C++])
AC_CHECK_HEADER([boost/shared_ptr.hpp], [],
                [AC_MSG_ERROR(Cannot find boost/shared_ptr header); exit])
AC_CHECK_HEADER([boost/graph/adjacency_list.hpp], [],
                [AC_MSG_ERROR(Cannot find boost/graph headers); exit])
AC_CHECK_HEADER([boost/ptr_container/indirect_fun.hpp], [],
                [AC_MSG_ERROR(Cannot find boost/ptr_container headers); exit])
AC_LANG_POP([C++])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

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
DX_INIT_DOXYGEN([dlvhex], [doxygen.cfg], [documentation])


# Checks for external libraries
AM_PATH_CPPUNIT([1.10.2])
AC_PATH_BOOST

AC_CONFIG_FILES([
           Makefile 
           dlvhex.pc
           examples/Makefile
           include/common.h
           include/dlvhex/Makefile
           include/testsuite/dlvhex/Makefile
           src/dlvhex/Makefile
           src/testsuite/dlvhex/Makefile
])


AC_OUTPUT

