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


noinst_PROGRAMS = easy1 matrix1

easy1_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/easy1.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

matrix1_SOURCES = \
	$(top_srcdir)/testsuite/benchgen/matrix1.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp
