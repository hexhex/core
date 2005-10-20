
AC_PREREQ([2.59])
AC_INIT([dlvhex],[0.1],[roman@kr.tuwien.ac.at])
AC_CONFIG_SRCDIR([src/dlvhex/dlvhex.cpp])
AC_CONFIG_HEADER([src/config.h])

#
# set system-wide plugin dir
#
CPPFLAGS="$CPPFLAGS -DPLUGIN_DIR='\"\$(pkglibdir)\"'"

#
# this has to be done properly:
# - checking if the optionally specified dlv exists at config-time
# - displaying some line for --help
#
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
#
# these are the defaults, can be overridden by --enable-doxygen-...
DX_HTML_FEATURE([ON])
DX_CHM_FEATURE([OFF])
DX_CHI_FEATURE([OFF])
DX_MAN_FEATURE([OFF])
DX_RTF_FEATURE([OFF])
DX_XML_FEATURE([OFF])
DX_PDF_FEATURE([OFF])
DX_PS_FEATURE([ON])
DX_INIT_DOXYGEN([dlvhex], [doxygen.cfg], [documentation])


AM_PATH_CPPUNIT([1.10.2])

AM_PATH_RAPTOR([1.3.2], [have_raptor=yes], [have_raptor=no])
AM_CONDITIONAL([HAVE_RAPTOR], [test "x$have_raptor" = xyes])

AM_PATH_ACE
AM_CONDITIONAL([HAVE_ACE], [test "x$ACE_LIBS" != x])

AC_CONFIG_FILES([
           Makefile 
           dlvhex.pc
	   examples/Makefile
           include/dlvhex/Makefile
           include/testsuite/dlvhex/Makefile
           src/dlvhex/Makefile
           src/testsuite/dlvhex/Makefile
])
#           include/extModules/racer/Makefile
#           src/extModules/test/Makefile
#           src/extModules/rdf/Makefile
#           src/extModules/racer/Makefile 
#           include/testsuite/extModules/racer/Makefile
#           src/testsuite/extModules/racer/Makefile

if test "$have_raptor" = "no"; then
    AC_MSG_NOTICE([])
    AC_MSG_NOTICE([libraptor not found or too old (needed: >= 1.3.2)])
    AC_MSG_NOTICE([the RDF-library will not be built!])
    AC_MSG_NOTICE([])
fi

AC_OUTPUT

#AC_MSG_RESULT([
#dlvhex build summary:
#  RDF library available     : $have_raptor
#])
