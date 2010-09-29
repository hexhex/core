# global preprocessor flags
AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    $(BOOST_CPPFLAGS) \
    $(DLVHEX_CFLAGS) \
    -DBOOST_TEST_DYN_LINK

# global linker flags
AM_LDFLAGS = $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS)

# global libs
LIBS = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

TESTS = \
  TestTypes \
  TestModelGraph TestModelGraphND \
  TestEvalGraph TestEvalGraphND \
  TestOnlineModelBuilder TestOnlineModelBuilderND \
  TestOfflineModelBuilder TestOfflineModelBuilderND

TESTS_ENVIRONMENT = EXAMPLESDIR=$(top_srcdir)/examples BOOST_TEST_REPORT_LEVEL=detailed BOOST_TEST_LOG_LEVEL=all

#TEST_SUITE_LOG = testsuite.log

check_PROGRAMS = \
  TestTypes \
  TestModelGraph TestModelGraphND \
  TestEvalGraph TestEvalGraphND \
  TestOnlineModelBuilder TestOnlineModelBuilderND \
  TestOfflineModelBuilder TestOfflineModelBuilderND


TestTypes_SOURCES = TestTypes.cpp 
TestTypes_LDADD = \
	@LIBLTDL@ @LIBADD_DL@ 

# test it with full debugging output
TestModelGraph_SOURCES = TestModelGraph.cpp dummytypes.cpp fixtureE2M2.cpp fixtureE2.cpp $(top_srcdir)/src/dlvhex/Logger.cpp

TestEvalGraph_SOURCES = TestEvalGraph.cpp dummytypes.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp

TestOnlineModelBuilder_SOURCES = TestOnlineModelBuilder.cpp dummytypes.cpp fixtureE1.cpp fixtureE2.cpp $(top_srcdir)/src/dlvhex/Logger.cpp

TestOfflineModelBuilder_SOURCES = TestOfflineModelBuilder.cpp dummytypes.cpp fixtureE1.cpp fixtureE2.cpp $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp

# test it with NDEBUG (the real situation)
TestModelGraphND_SOURCES = $(TestModelGraph_SOURCES)
TestModelGraphND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestEvalGraphND_SOURCES = $(TestEvalGraph_SOURCES)
TestEvalGraphND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestOnlineModelBuilderND_SOURCES = $(TestOnlineModelBuilder_SOURCES)
TestOnlineModelBuilderND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestOfflineModelBuilderND_SOURCES = $(TestOfflineModelBuilder_SOURCES)
TestOfflineModelBuilderND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG
