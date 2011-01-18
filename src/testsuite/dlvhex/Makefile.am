CLEANFILES = libdlvhextest.so

EXTRA_DIST = run-dlvhex-tests.sh

##
## this is a bit of a hack
## automake does not allow shared libraries in noinst_LTLIBRARIES and
## check_LTLIBRARIES. No matter what, we only get archive files. (They
## call it "convinience libraries" in the documentation.) To overcome
## this "feature", we declare libdlvhextest.la as lib_LTLIBRARIES and
## trick automake into believing that we want to install
## libdlvhextest.la into $prefix/lib. In the end, we redefine the
## install-exec-am Makefile target and install nothing at all.
##
lib_LTLIBRARIES = libdlvhextest.la
install-exec-am:

all-local:
	$(RM) libdlvhextest.so; $(LN_S) .libs/libdlvhextest.so .

libdlvhextest_la_SOURCES = TestPlugin.cpp

libdlvhextest_la_LDFLAGS = -avoid-version -module
