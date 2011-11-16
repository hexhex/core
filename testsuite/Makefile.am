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

# full debugging output

# FIXME this should be partitioned into $(top_builddir)/src/dlvhex/lib{X,Y,Z}.a
HEXSOURCES = \
	$(top_srcdir)/src/dlvhex/AnswerSet.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolverManager.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/BaseModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp \
	$(top_srcdir)/src/dlvhex/ComponentGraph.cpp \
	$(top_srcdir)/src/dlvhex/Configuration.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraphFull.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/DLVProcess.cpp \
	$(top_srcdir)/src/dlvhex/DLVresultParserDriver.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp \
	$(top_srcdir)/src/dlvhex/EvalGraphBuilder.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicEasy.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicOldDlvhex.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicTrivial.cpp \
	$(top_srcdir)/src/dlvhex/GuessAndCheckModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammar.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/InputProvider.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/PlainModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/PluginContainer.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/PredicateMask.cpp \
	$(top_srcdir)/src/dlvhex/Printer.cpp \
	$(top_srcdir)/src/dlvhex/ProcessBuf.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Registry.cpp \
	$(top_srcdir)/src/dlvhex/URLBuf.cpp \
	$(top_srcdir)/src/dlvhex/WellfoundedModelGenerator.cpp

check_LIBRARIES = libhex.a

libhex_a_SOURCES = $(HEXSOURCES)

TestPlainHEX_SOURCES = $(top_srcdir)/testsuite/TestPlainHEX.cpp
TestPlainHEX_LDADD = libhex.a $(EXTSOLVER_LDFLAGS) $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LIBS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

TestEvalEndToEnd_SOURCES = $(top_srcdir)/testsuite/TestEvalEndToEnd.cpp
TestEvalEndToEnd_LDADD = libhex.a $(EXTSOLVER_LDFLAGS) $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LIBS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

TestASPSolver_SOURCES = $(top_srcdir)/testsuite/TestASPSolver.cpp
TestASPSolver_LDADD = libhex.a $(EXTSOLVER_LDFLAGS) $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LIBS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

#TODO evalheuristic should (and could) be tested with fake model generator and fake interpretation
TestEvalHeuristic_SOURCES = $(top_srcdir)/testsuite/TestEvalHeuristic.cpp
TestEvalHeuristic_LDADD = libhex.a $(EXTSOLVER_LDFLAGS) $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LIBS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

TestComponentGraph_SOURCES = $(top_srcdir)/testsuite/TestComponentGraph.cpp
TestComponentGraph_LDADD = libhex.a $(EXTSOLVER_LDFLAGS) $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LIBS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

TestDependencyGraph_SOURCES = $(top_srcdir)/testsuite/TestDependencyGraph.cpp
TestDependencyGraph_LDADD = libhex.a $(EXTSOLVER_LDFLAGS) $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LIBS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

TestHexParser_SOURCES = $(top_srcdir)/testsuite/TestHexParser.cpp
TestHexParser_LDADD = libhex.a $(EXTSOLVER_LDFLAGS) $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LIBS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

TestHexParserModule_SOURCES = \
	$(top_srcdir)/testsuite/TestHexParserModule.cpp \
	$(top_srcdir)/src/dlvhex/MLPSyntaxChecker.cpp
TestHexParserModule_LDADD = libhex.a $(BOOST_THREAD_LDFLAGS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@ 

TestMLPSolver_SOURCES = \
	$(top_srcdir)/testsuite/TestMLPSolver.cpp \
	$(top_srcdir)/src/dlvhex/MLPSyntaxChecker.cpp \
	$(top_srcdir)/src/dlvhex/MLPSolver.cpp
TestMLPSolver_LDADD = libhex.a $(EXTSOLVER_LDFLAGS) $(BOOST_THREAD_LDFLAGS) $(EXTSOLVER_LIBS) $(BOOST_THREAD_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@
TestMLPSolver_CPPFLAGS = $(EXTSOLVER_CPPFLAGS) 

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
