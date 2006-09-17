AUTOMAKE_OPTIONS = foreign

EXTRA_DIST = run-dlvhex-tests.sh

TESTS = dlvhexTS run-dlvhex-tests.sh

TESTS_ENVIRONMENT = EXAMPLES=$(top_srcdir)/examples/tests

check_PROGRAMS = dlvhexTS

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
