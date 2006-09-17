AUTOMAKE_OPTIONS = foreign

TESTS = dlvhexTS

check_PROGRAMS = $(TESTS)

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

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src

dlvexTS_CXXFLAGS = $(CPPUNIT_CFLAGS)

dlvhexTS_LDFLAGS = $(CPPUNIT_LIBS) -ldl

dlvhexTS_LDADD = \
                $(top_builddir)/src/dlvhex/libdlvhexbase.la \
                $(top_builddir)/src/dlvhex/libaspsolver.la
