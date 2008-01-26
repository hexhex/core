AUTOMAKE_OPTIONS = gnu check-news
ACLOCAL_AMFLAGS = -I m4
include doxygen.am

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = dlvhex.pc

SUBDIRS = libltdl examples include src

EXTRA_DIST = \
    COPYING.LESSER \
    dlvhex.pc.in \
    doxygen.am \
    doxygen.cfg

MOSTLYCLEANFILES = $(DX_CLEANFILES)

