
AC_PREREQ([2.59])
AC_INIT([dlvex],[0.1],[roman@kr.tuwien.ac.at])
AC_CONFIG_SRCDIR([src/dlvex/dlvex.cpp])
AC_CONFIG_HEADER([src/config.h])

#
# set system-wide plugin dir
#
CPPFLAGS="$CPPLAGS -DPLUGIN_DIR='\$(pkglibdir)'"

if test "$DLVPATH" = ""; then
    DLVPATH=dlv
fi
    
AC_DEFINE_UNQUOTED(DLVPATH,"$DLVPATH",[Path of DLV executable])

if test "$RACERPATH" = ""; then
    RACERPATH=RacerPro
fi
    
AC_DEFINE_UNQUOTED(RACERPATH,"$RACERPATH",[Path of RACER executable])


AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_LEX
AC_PROG_YACC

#
# this has to be done properly:
# - checking if the optionally specified dlv exists at config-time
# - displaying some line for --help
#

#AC_CHECK_PROG([have_dlv], ${}, [yes], [no])
#if test "$have_dlv" = "no"; then
#    AC_MSG_ERROR([dlv not found! \
#The Answer-Set Solver DLV is needed for dlvex. It can be obtained at \
#http://www.dbai.tuwien.ac.at/proj/dlv/#download. If you have it on \
#your system, be sure, that it is executable under the name dlv!])
#fi

# Checks for libraries.

# Checks for header files.
AC_HEADER_DIRENT
AC_CHECK_HEADERS([sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST

# Checks for library functions.
AC_PROG_LIBTOOL

# doxygen stuff
DX_HTML_FEATURE([ON])
DX_CHM_FEATURE([OFF])
DX_CHI_FEATURE([OFF])
DX_MAN_FEATURE([OFF])
DX_RTF_FEATURE([OFF])
DX_XML_FEATURE([OFF])
DX_PDF_FEATURE([OFF])
DX_PS_FEATURE([OFF])
DX_INIT_DOXYGEN([dlvex], [doxygen.cfg], [documentation])

#AC_CONFIG_FILES([src/dlvex/Makefile])

AM_PATH_CPPUNIT([1.10.2])

AM_PATH_RAPTOR([1.3.2], [have_raptor=yes], [have_raptor=no])
AM_CONDITIONAL([HAVE_RAPTOR], [test "x$have_raptor" = xyes])

AM_PATH_ACE
AM_CONDITIONAL([HAVE_ACE], [test "x$ACE_LIBS" != x])

AC_CONFIG_FILES([\
           Makefile \
	   include/dlvex/Makefile \
	   include/extModules/racer/Makefile \
	   include/testsuite/dlvex/Makefile \
	   include/testsuite/extModules/racer/Makefile \
           src/dlvex/Makefile \
           src/testsuite/dlvex/Makefile \
           src/testsuite/extModules/racer/Makefile \
           src/extModules/rdf/Makefile \
           src/extModules/racer/Makefile \
])

if test "$have_raptor" = "no"; then
    AC_MSG_NOTICE([])
    AC_MSG_NOTICE([libraptor not found or too old (needed: >= 1.3.2)])
    AC_MSG_NOTICE([the RDF-library will not be built!])
    AC_MSG_NOTICE([])
fi

AC_OUTPUT

#AC_MSG_RESULT([
#dlvex build summary:
#  RDF library available     : $have_raptor
#])
