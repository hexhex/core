CLEANFILES = libdlvhextest.so

EXTRA_DIST = run-dlvhex-tests.sh

if HAVE_CPPUNIT
TESTS = dlvhexTS run-dlvhex-tests.sh
TESTS_ENVIRONMENT = TESTDIR=$(top_srcdir)/examples/tests LD_LIBRARY_PATH="$(top_builddir)/src/dlvhex/.libs/:${LD_LIBRARY_PATH}" DLVHEX=$(top_builddir)/src/dlvhex/dlvhex PARAMETERS="-s --plugindir=. --solver=dlvlib"
check_PROGRAMS = dlvhexTS
else
check_PROGRAMS = 
endif

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

dlvhexTS_SOURCES = \
    TestSuite.cpp \
    TypeTest.cpp \
    TestGraphProcessor.cpp \
    ASPsolverTest.cpp 

AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    $(BOOST_CPPFLAGS) \
    $(DLVHEX_CFLAGS)

dlvexTS_CXXFLAGS = $(CPPUNIT_CFLAGS)

dlvhexTS_LDFLAGS = $(CPPUNIT_LIBS) $(DLVHEX_LIBS) @LIBCURL@

dlvhexTS_LDADD = \
    @LIBLTDL@ @LIBADD_DL@ \
    $(top_builddir)/src/dlvhex/libdlvhexbase.la \
    $(top_builddir)/src/dlvhex/libaspsolver.la


libdlvhextest_la_SOURCES = TestPlugin.cpp

libdlvhextest_la_LDFLAGS = -avoid-version -module
