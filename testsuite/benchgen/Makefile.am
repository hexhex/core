# global preprocessor flags
AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    $(DLVHEX_CFLAGS) \
    $(BOOST_CPPFLAGS)

# global linker flags
#AM_LDFLAGS = $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS) #-lcwd

# global libs
#LIBS = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

AM_LDFLAGS = $(BOOST_DATE_TIME_LDFLAGS) $(BOOST_PROGRAM_OPTIONS_LDFLAGS) $(BOOST_SYSTEM_LDFLAGS) $(BOOST_THREAD_LDFLAGS) $(BOOST_SERIALIZATION_LDFLAGS) $(BOOST_GRAPH_LDFLAGS)
#AM_LDFLAGS = $(BOOST_PROGRAM_OPTIONS_LDFLAGS)

# global libs
LIBS = $(BOOST_DATE_TIME_LIBS) $(BOOST_PROGRAM_OPTIONS_LIBS) $(BOOST_SYSTEM_LIBS) $(BOOST_THREAD_LIBS) $(BOOST_SERIALIZATION_LIBS) $(BOOST_GRAPH_LIBS)
#LIBS = $(BOOST_PROGRAM_OPTIONS_LIBS)


noinst_PROGRAMS = easy1 matrix1 matrix2 bomb1 bomb2 bomb3 revsel1 revsel2

easy1_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/easy1.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

matrix1_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/matrix1.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

matrix2_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/matrix2.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

bomb1_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/bomb1.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

bomb2_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/bomb2.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

bomb3_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/bomb3.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

revsel1_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/revsel1.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

revsel2_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/revsel2.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp
