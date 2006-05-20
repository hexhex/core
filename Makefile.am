ACLOCAL_AMFLAGS = -I m4
include doxygen.am

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = dlvhex.pc

SUBDIRS = \
          include/dlvhex \
          include/testsuite/dlvhex \
          src/dlvhex \
          src/testsuite/dlvhex

EXTRA_DIST = \
    dlvhex.pc.in \
    doxygen.am \
    doxygen.cfg

MOSTLYCLEANFILES = $(DX_CLEANFILES)

