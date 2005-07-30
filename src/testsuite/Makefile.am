AUTOMAKE_OPTIONS = foreign

TESTS = dlvexTS

check_PROGRAMS = $(TESTS)

dlvexTS_SOURCES = \
    TestSuite.cpp \
    TypeTest.cpp \
    ASPsolverTest.cpp
    
AM_CPPFLAGS = -I$(top_srcdir)/include

dlvexTS_CXXFLAGS = $(CPPUNIT_CFLAGS)

dlvexTS_LDFLAGS = $(CPPUNIT_LIBS) -ldl

dlvexTS_LDADD = \
                ../dlvex/libtypes.la \
                ../dlvex/libaspsolver.la
