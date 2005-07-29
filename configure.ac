
AC_PREREQ([2.59])
AC_INIT([dlvex],[0.1],[roman@kr.tuwien.ac.at])
AC_CONFIG_SRCDIR([src/dlvex/dlvex.cpp])
#AC_CONFIG_HEADER([src/config.h])

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

AM_PATH_RAPTOR([1.4.7], [have_raptor=yes], [have_raptor=no])
AM_CONDITIONAL(HAVE_RAPTOR, test x$have_raptor = xyes)

AC_CONFIG_FILES([\
           Makefile \
           src/dlvex/Makefile \
           src/testsuite/Makefile \
           src/extModules/rdf/Makefile \
])
AC_OUTPUT
