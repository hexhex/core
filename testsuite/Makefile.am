# global preprocessor flags
AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    $(BOOST_CPPFLAGS) \
    $(DLVHEX_CFLAGS)

# global linker flags
AM_LDFLAGS = $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS)

# global libs
LIBS = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

TESTS = \
	TestTypes

TESTS_ENVIRONMENT = EXAMPLESDIR=$(top_srcdir)/examples

check_PROGRAMS = \
	TestTypes

TestTypes_SOURCES = TestTypes.cpp 
TestTypes_LDADD = \
	@LIBLTDL@ @LIBADD_DL@ 
