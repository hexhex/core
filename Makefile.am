# use at least automake >= 1.11
AUTOMAKE_OPTIONS = 1.11

# use ./m4 directory for additional macros
ACLOCAL_AMFLAGS = -I m4

# setup doxygen for automake use
include doxygen.am

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = dlvhex.pc

html-local: doc/dlvhex.tag
	make doxygen-doc

install-html-local:
	install -d $(DESTDIR)/$(docdir)
	cp -rv $(top_builddir)/doc/html $(DESTDIR)/$(docdir)

uninstall-local:
	test -d $(DESTDIR)/$(docdir)/html || exit 0; \
	cd $(DESTDIR)/$(docdir) && rm -rf html


SUBDIRS = libltdl examples include src

EXTRA_DIST = \
    COPYING.LESSER \
    dlvhex.pc.in \
    doxygen.am \
    doxygen.cfg

MOSTLYCLEANFILES = $(DX_CLEANFILES)

