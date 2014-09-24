# use at least automake >= 1.10
AUTOMAKE_OPTIONS = 1.10

# use ./m4 directory for additional macros
ACLOCAL_AMFLAGS = -I m4

# setup doxygen for automake use
include doxygen.am

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = dlvhex2.pc

html-local: doc/dlvhex.tag
	make doxygen-doc

install-html-local:
	$(INSTALL) -d $(DESTDIR)/$(docdir)
	cp -rv $(top_builddir)/doc/html $(DESTDIR)/$(docdir)

pkgincludedir = $(includedir)/dlvhex2
install-data-local:
	if test x$(BM_INSTALL) == xyes; then \
	  $(INSTALL) -d $(DESTDIR)$(pkgincludedir)/bm$(BM_VERSION)/bm; \
	  $(INSTALL) $(top_srcdir)/bm$(BM_VERSION)/bm/*.h $(DESTDIR)/$(pkgincludedir)/bm$(BM_VERSION)/bm/; \
	fi

uninstall-local:
	test -d $(DESTDIR)/$(docdir)/html || exit 0; \
	cd $(DESTDIR)/$(docdir) && rm -rf html
	-rm -rf $(DESTDIR)/$(pkgincludedir)/bm$(BM_VERSION)

SUBDIRS = ${BUILDCLASPGRINGOSUBDIRS} \
  libltdl examples include src testsuite
DIST_SUBDIRS = buildclaspgringo \
  libltdl examples include src testsuite

EXTRA_DIST = \
    bm3.7.0 \
    COPYING.LESSER \
    COPYING.clasp.gringo \
    clasp-3.1.0-source.tar.gz \
    gringo-4.4.0-source.tar.gz \
    build_potassco.sh \
    dlvhex2.pc.in \
    doxygen.am \
    doxygen.cfg

MOSTLYCLEANFILES = $(DX_CLEANFILES)

