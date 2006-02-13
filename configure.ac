
AC_PREREQ([2.59])
AC_INIT([dlvhex],[0.1],[roman@kr.tuwien.ac.at])
AC_CONFIG_SRCDIR([src/dlvhex/dlvhex.cpp])
AC_CONFIG_HEADER([src/config.h])

#
# set system-wide plugin dir
#
#CPPFLAGS="$CPPFLAGS -DPLUGIN_DIR='\"\$(pkglibdir)\"'"

#
# the default system-wide plugin dir $(pkglibdir)\plugins can be
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
    # and check if and where it exists
    AC_PATH_PROG(DLVPATH, $DLV, notfound)
else
    # the user supplied a 'DLV=...'
    # check if it exists (with search path=/, since the user supplied
    # an entire path!) and set DLVPATH to it!
    AC_CHECK_PROG(DLVPATH, $DLV, $DLV, notfound, [/])
fi

if test "$DLVPATH" = notfound; then
    AC_MSG_ERROR([dlv not found])
fi

#
# put the dlv path into config.h
#
AC_DEFINE_UNQUOTED(DLVPATH,"$DLVPATH",[Path of DLV executable])


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

AC_CONFIG_FILES([
           Makefile 
           dlvhex.pc
	   examples/Makefile
           include/dlvhex/Makefile
           include/testsuite/dlvhex/Makefile
           src/dlvhex/Makefile
           src/testsuite/dlvhex/Makefile
])


AC_OUTPUT

