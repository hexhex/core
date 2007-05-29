AUTOMAKE_OPTIONS = gnu

EXTRA_DIST = run-dlvhex-tests.sh

TESTS = dlvhexTS run-dlvhex-tests.sh

TESTS_ENVIRONMENT = TESTDIR=$(top_srcdir)/examples/tests DLVHEX=$(top_builddir)/src/dlvhex/dlvhex PARAMETERS="-s --plugindir=."

check_PROGRAMS = dlvhexTS

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
    ASPsolverTest.cpp \
    $(top_srcdir)/src/dlvhex/BoostComponentFinder.cpp \
    $(top_srcdir)/src/dlvhex/GraphProcessor.cpp \
    $(top_srcdir)/src/dlvhex/GraphBuilder.cpp \
    $(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
    $(top_srcdir)/src/dlvhex/AtomNode.cpp \
    $(top_srcdir)/src/dlvhex/Component.cpp \
    $(top_srcdir)/src/dlvhex/OrdinaryModelGenerator.cpp \
    $(top_srcdir)/src/dlvhex/FixpointModelGenerator.cpp \
    $(top_srcdir)/src/dlvhex/GuessCheckModelGenerator.cpp \
    $(top_srcdir)/src/dlvhex/ProgramBuilder.cpp

AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    $(DLVHEX_CFLAGS)

dlvexTS_CXXFLAGS = $(CPPUNIT_CFLAGS)

dlvhexTS_LDFLAGS = $(CPPUNIT_LIBS) $(DLVHEX_LIBS)

dlvhexTS_LDADD = \
    $(top_builddir)/src/dlvhex/libdlvhexbase.la \
    $(top_builddir)/src/dlvhex/libaspsolver.la


libdlvhextest_la_SOURCES = TestPlugin.cpp

libdlvhextest_la_LDFLAGS = -avoid-version -module
