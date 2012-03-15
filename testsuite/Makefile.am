EXTRA_DIST = run-dlvhex-tests.sh answerset_compare.py run_TestDLVProcess.sh

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

check_LTLIBRARIES = libdlvhexplugin-test.la

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
  TestPlainHEX \
  TestDLVProcess

TESTS = \
  run-dlvhex-tests.sh \
  $(AUTOMATED_TEST_PROGS) \
  run_TestDLVProcess.sh

TESTS_ENVIRONMENT = \
  TOP_BUILDDIR=$(top_srcdir) \
  TOP_SRCDIR=$(top_srcdir) \
  EXAMPLESDIR=$(top_srcdir)/examples \
  TESTS="$(WHICH_DLVHEX_TESTS)" \
  OUTDIR=$(top_srcdir)/examples/tests \
  DLVHEX="$(top_builddir)/src/dlvhex2 -s --plugindir=!$(top_builddir)/testsuite" \
  BOOST_TEST_REPORT_LEVEL=detailed \
  BOOST_TEST_LOG_LEVEL=all

# remove .dot files produced by running check_PROGRAMS
clean-local:
	-rm -rf test*{Verbose,Terse}.dot

noinst_HEADERS = \
  dummytypes.h \
  graphviz.h \
  fixturesMCS.h \
  fixtureE1.h \
  fixtureE2.h \
  fixtureE2M2.h \
  fixtureEx1.h \
  fixturesExt1.h \
  fixturesDepgraphCompgraphGeneric.h \
  fixtureOfflineMB.h \
  fixtureOnlineMB.h

libdlvhexplugin_test_la_SOURCES = TestPlugin.cpp
# this rpath is specially required as this lib is no plugin_LTLIBRARY but a check_LTLIBRARY
# (if it is a plugin_LTLIBRARY it will be installed and we do not want to install it)
# (check_plugin_LTLIBRARY does not work at the moment)
libdlvhexplugin_test_la_LDFLAGS = $(AM_LDFLAGS) -avoid-version -module -rpath /

#this is just to verify that TestPlugin can link to the rest of the code
#if this does not work, the TestPlugin cannot be dlopen'ed and all tests are bound to fail
TestTestPluginStatic_SOURCES = \
  TestPlugin.cpp \
  $(top_srcdir)/src/dlvhex.cpp \
  $(top_srcdir)/src/State.cpp 
TestTestPluginStatic_LDADD = \
  $(top_builddir)/src/libdlvhex2-base.la \
  $(top_builddir)/src/libdlvhex2-mlpsolver.la \
  $(top_builddir)/src/libdlvhex2-aspsolver.la \
  $(top_builddir)/src/libdlvhex2-internalplugins.la \
  $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LDFLAGS) \
  $(BOOST_THREAD_LIBS) $(EXTSOLVER_LIBS) \
  @LIBLTDL@ @LIBADD_DL@ @LIBCURL@
# the following line is just to make sure TestPlugin.cpp will be built separately
# for libdlvhextest_la and for TestTestPluginStatic (seems like a automake/libtool bug)
TestTestPluginStatic_CPPFLAGS = $(AM_CXXFLAGS)

# libdlvhexbase does not depend on libaspsolver and does not depend on libmlpsolver
# libaspsolver does not depend on libmlpsolver
LDADD_BASE = $(top_builddir)/src/libdlvhex2-base.la $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) $(EXTSOLVER_LDFLAGS) $(EXTSOLVER_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@
LDADD_ASPSOLVER = $(top_builddir)/src/libdlvhex2-aspsolver.la $(LDADD_BASE) $(EXTSOLVER_LDFLAGS) $(EXTSOLVER_LIBS)
LDADD_MLP_ASPSOLVER = $(top_builddir)/src/libdlvhex2-mlpsolver.la $(LDADD_ASPSOLVER)

TestPlainHEX_SOURCES = TestPlainHEX.cpp
TestPlainHEX_LDADD = $(LDADD_ASPSOLVER)

TestEvalEndToEnd_SOURCES = TestEvalEndToEnd.cpp
TestEvalEndToEnd_LDADD = $(LDADD_ASPSOLVER)

TestASPSolver_SOURCES = TestASPSolver.cpp
TestASPSolver_LDADD = $(LDADD_ASPSOLVER)

#TODO evalheuristic should (and could) be tested with fake model generator and fake interpretation
TestEvalHeuristic_SOURCES = TestEvalHeuristic.cpp
TestEvalHeuristic_LDADD = $(LDADD_ASPSOLVER)

TestComponentGraph_SOURCES = TestComponentGraph.cpp
TestComponentGraph_LDADD = $(LDADD_BASE)

TestDependencyGraph_SOURCES = TestDependencyGraph.cpp $(top_srcdir)/src/DependencyGraphFull.cpp
TestDependencyGraph_LDADD = $(LDADD_BASE)

TestHexParser_SOURCES = TestHexParser.cpp
TestHexParser_LDADD = $(LDADD_BASE)

# TODO why do we need MLP here?
TestHexParserModule_SOURCES = TestHexParserModule.cpp
TestHexParserModule_LDADD = $(LDADD_MLP_ASPSOLVER)

TestMLPSolver_SOURCES = $(top_srcdir)/testsuite/TestMLPSolver.cpp
TestMLPSolver_LDADD = $(LDADD_MLP_ASPSOLVER)

TestTables_SOURCES = \
	TestTables.cpp \
	$(top_srcdir)/src/Logger.cpp \
	$(top_srcdir)/src/ID.cpp
TestTables_LDADD = $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ 

TestModelGraph_SOURCES = \
	TestModelGraph.cpp \
	dummytypes.cpp \
	fixtureE2M2.cpp \
	fixtureE2.cpp \
	$(top_srcdir)/src/Logger.cpp

TestEvalGraph_SOURCES = \
	TestEvalGraph.cpp \
	dummytypes.cpp \
	fixtureE2.cpp \
	fixtureEx1.cpp \
	$(top_srcdir)/src/Logger.cpp \
	$(top_srcdir)/src/CAUAlgorithms.cpp

TestOnlineModelBuilder_SOURCES = \
	TestOnlineModelBuilder.cpp \
	dummytypes.cpp \
	fixtureE1.cpp \
	fixtureE2.cpp \
	fixtureEx1.cpp \
	$(top_srcdir)/src/Logger.cpp

TestOfflineModelBuilder_SOURCES = \
	TestOfflineModelBuilder.cpp \
	dummytypes.cpp \
	fixtureE1.cpp \
	fixtureE2.cpp \
	fixtureEx1.cpp \
	$(top_srcdir)/src/Logger.cpp \
	$(top_srcdir)/src/CAUAlgorithms.cpp

TestDLVProcess_SOURCES = \
	TestDLVProcess.cpp
TestDLVProcess_LDADD = $(LDADD_ASPSOLVER)
