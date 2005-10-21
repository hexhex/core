ACLOCAL_AMFLAGS = -I m4
include doxygen.am

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = dlvhex.pc

EXTRA_DIST = dlvhex.pc.in

SUBDIRS = \
          examples \
          include/dlvhex \
          include/testsuite/dlvhex \
          src/dlvhex \
          src/testsuite/dlvhex
