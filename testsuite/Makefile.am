EXTRA_DIST = run-dlvhex-tests.sh answerset_compare.py

# global preprocessor flags
AM_CXXFLAGS = \
    -I$(top_srcdir)/include \
    $(BOOST_CPPFLAGS) \
    $(BM_CPPFLAGS) \
    $(DLVHEX_CFLAGS) \
    $(LIBCURL_CPPFLAGS) \
    $(EXTSOLVER_CPPFLAGS) \
    -DBOOST_TEST_DYN_LINK

    #-DCWDEBUG

# global linker flags
AM_LDFLAGS = $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS) #-lcwd

# global libs
LIBS = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

lib_LTLIBRARIES = libdlvhextest.la

check-examples:
	if $(TESTS_ENVIRONMENT) $(srcdir)/run-dlvhex-tests.sh; then \
		echo "PASS"; \
	else \
		echo "FAIL"; \
	fi

AUTOMATED_TEST_PROGS = \
  TestASPSolver \
  TestEvalEndToEnd \
  TestEvalHeuristic \
  TestComponentGraph \
  TestDependencyGraph \
  TestHexParser \
  TestHexParserModule \
  TestMLPSolver \
  TestTables \
  TestModelGraph \
  TestEvalGraph \
  TestOnlineModelBuilder \
  TestOfflineModelBuilder 

check_PROGRAMS =  \
  $(AUTOMATED_TEST_PROGS) \
  TestTestPluginStatic \
  TestPlainHEX

TESTS = \
  run-dlvhex-tests.sh \
  $(AUTOMATED_TEST_PROGS)

TESTS_ENVIRONMENT = \
  TOP_SRCDIR=$(top_srcdir) \
  EXAMPLESDIR=$(top_srcdir)/examples \
  TESTDIR=$(top_srcdir)/examples/tests \
  DLVHEX="$(top_builddir)/src/dlvhex/dlvhex -s --plugindir=!$(top_builddir)/testsuite" \
  BOOST_TEST_REPORT_LEVEL=detailed \
  BOOST_TEST_LOG_LEVEL=all

# remove .dot files produced by running check_PROGRAMS
clean-local:
	-rm -rf test*{Verbose,Terse}.dot

noinst_HEADERS = \
  dummytypes.hpp \
  graphviz.hpp \
  fixturesMCS.hpp \
  fixtureE1.hpp \
  fixtureE2.hpp \
  fixtureE2M2.hpp \
  fixtureEx1.hpp \
  fixturesExt1.hpp \
  fixturesDepgraphCompgraphGeneric.hpp \
  fixtureOfflineMB.hpp \
  fixtureOnlineMB.hpp

libdlvhextest_la_SOURCES = TestPlugin.cpp
# this rpath is specially required as this lib is no plugin_LTLIBRARY but a check_LTLIBRARY
libdlvhextest_la_LDFLAGS = $(AM_LDFLAGS) -avoid-version -module #-rpath /
libdlvhextest_la_LIBADD = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

#this is just to verify that TestPlugin can link to the rest of the code
#if this does not work, the TestPlugin cannot be dlopen'ed and all tests are bound to fail
TestTestPluginStatic_SOURCES = TestPlugin.cpp $(top_srcdir)/src/dlvhex/dlvhex.cpp 
TestTestPluginStatic_LDADD = \
  $(top_builddir)/src/dlvhex/libdlvhexbase.la \
  $(top_builddir)/src/dlvhex/libmlpsolver.la \
  $(top_builddir)/src/dlvhex/libaspsolver.la \
  $(top_builddir)/src/dlvhex/libinternalplugins.la \
  $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LDFLAGS) \
  $(BOOST_THREAD_LIBS) $(EXTSOLVER_LIBS) \
  @LIBLTDL@ @LIBADD_DL@ @LIBCURL@
# the following line is just to make sure TestPlugin.cpp will be built separately
# for libdlvhextest_la and for TestTestPluginStatic (seems like a automake/libtool bug)
TestTestPluginStatic_CPPFLAGS = $(AM_CXXFLAGS)

LDADD_WITHOUT_EXTSOLVER = $(top_builddir)/src/dlvhex/libmlpsolver.la $(top_builddir)/src/dlvhex/libdlvhexbase.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@
LDADD_WITH_EXTSOLVER = $(top_builddir)/src/dlvhex/libaspsolver.la $(EXTSOLVER_LDFLAGS) $(EXTSOLVER_LIBS) $(LDADD_WITHOUT_EXTSOLVER)
# TODO review which code goes into which library, it should be libdlvhexbase.la and libaspsolver do not depend on each other, and both do not depend on libmlpsolver.la

TestPlainHEX_SOURCES = TestPlainHEX.cpp
TestPlainHEX_LDADD = $(LDADD_WITH_EXTSOLVER)

TestEvalEndToEnd_SOURCES = TestEvalEndToEnd.cpp
TestEvalEndToEnd_LDADD = $(LDADD_WITH_EXTSOLVER)

TestASPSolver_SOURCES = TestASPSolver.cpp
TestASPSolver_LDADD = $(LDADD_WITH_EXTSOLVER)

#TODO evalheuristic should (and could) be tested with fake model generator and fake interpretation
TestEvalHeuristic_SOURCES = TestEvalHeuristic.cpp
TestEvalHeuristic_LDADD = $(LDADD_WITH_EXTSOLVER)

TestComponentGraph_SOURCES = TestComponentGraph.cpp
TestComponentGraph_LDADD = $(LDADD_WITH_EXTSOLVER)

TestDependencyGraph_SOURCES = TestDependencyGraph.cpp $(top_srcdir)/src/dlvhex/DependencyGraphFull.cpp
TestDependencyGraph_LDADD = $(LDADD_WITH_EXTSOLVER)

TestHexParser_SOURCES = TestHexParser.cpp
TestHexParser_LDADD = $(LDADD_WITH_EXTSOLVER)

TestHexParserModule_SOURCES = TestHexParserModule.cpp
TestHexParserModule_LDADD = $(LDADD_WITH_EXTSOLVER)

TestMLPSolver_SOURCES = $(top_srcdir)/testsuite/TestMLPSolver.cpp
TestMLPSolver_LDADD = $(LDADD_WITH_EXTSOLVER)
#TestMLPSolver_CPPFLAGS = $(EXTSOLVER_CPPFLAGS) 

TestTables_SOURCES = \
	TestTables.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp
TestTables_LDADD = $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ 

TestModelGraph_SOURCES = \
	TestModelGraph.cpp \
	dummytypes.cpp \
	fixtureE2M2.cpp \
	fixtureE2.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

TestEvalGraph_SOURCES = \
	TestEvalGraph.cpp \
	dummytypes.cpp \
	fixtureE2.cpp \
	fixtureEx1.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp

TestOnlineModelBuilder_SOURCES = \
	TestOnlineModelBuilder.cpp \
	dummytypes.cpp \
	fixtureE1.cpp \
	fixtureE2.cpp \
	fixtureEx1.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp

TestOfflineModelBuilder_SOURCES = \
	TestOfflineModelBuilder.cpp \
	dummytypes.cpp \
	fixtureE1.cpp \
	fixtureE2.cpp \
	fixtureEx1.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp
