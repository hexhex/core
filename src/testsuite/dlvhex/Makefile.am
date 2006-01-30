AUTOMAKE_OPTIONS = foreign

TESTS = dlvhexTS

check_PROGRAMS = $(TESTS)

dlvhexTS_SOURCES = \
    TestSuite.cpp \
    TypeTest.cpp \
    ASPsolverTest.cpp
    
AM_CPPFLAGS = -I$(top_srcdir)/include

dlvexTS_CXXFLAGS = $(CPPUNIT_CFLAGS)

dlvhexTS_LDFLAGS = $(CPPUNIT_LIBS) -ldl

dlvhexTS_LDADD = \
                $(top_builddir)/src/dlvhex/libdlvhexbase.la \
                $(top_builddir)/src/dlvhex/libaspsolver.la
