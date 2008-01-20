AUTOMAKE_OPTIONS = gnu
ACLOCAL_AMFLAGS = -I m4
include doxygen.am

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = dlvhex.pc

SUBDIRS = libltdl examples include src

EXTRA_DIST = \
    dlvhex.pc.in \
    doxygen.am \
    doxygen.cfg

MOSTLYCLEANFILES = $(DX_CLEANFILES)

