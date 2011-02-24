EXTRA_DIST = run-dlvhex-tests.sh answerset_compare.py

# global preprocessor flags
AM_CXXFLAGS = \
    -I$(top_srcdir)/include \
    $(BOOST_CPPFLAGS) \
    $(BM_CPPFLAGS) \
    $(DLVHEX_CFLAGS) \
    -DBOOST_TEST_DYN_LINK

    #-DCWDEBUG

# global linker flags
AM_LDFLAGS = $(BOOST_UNIT_TEST_FRAMEWORK_LDFLAGS) #-lcwd

# global libs
LIBS = $(BOOST_UNIT_TEST_FRAMEWORK_LIBS)

lib_LTLIBRARIES = libdlvhextest.la

TESTS = \
  run-dlvhex-tests.sh \
  TestEvalEndToEnd TestEvalEndToEndND \
  TestEvalHeuristic TestEvalHeuristicND \
  TestComponentGraph TestComponentGraphND \
  TestDependencyGraph TestDependencyGraphND \
  TestHexParser TestHexParserND \
  TestTables TestTablesND \
  TestModelGraph TestModelGraphND \
  TestEvalGraph TestEvalGraphND \
  TestOnlineModelBuilder TestOnlineModelBuilderND \
  TestOfflineModelBuilder TestOfflineModelBuilderND

TESTS_ENVIRONMENT = TOPSRCDIR=$(top_srcdir) TOPBUILDDIR=$(top_builddir) BOOST_TEST_REPORT_LEVEL=detailed BOOST_TEST_LOG_LEVEL=all

check_PROGRAMS =  \
  TestTestPluginStatic \
  TestASPSolver TestASPSolverND \
  TestEvalEndToEnd TestEvalEndToEndND \
  TestEvalHeuristic TestEvalHeuristicND \
  TestComponentGraph TestComponentGraphND \
  TestDependencyGraph TestDependencyGraphND \
  TestHexParser TestHexParserND \
  TestTables TestTablesND \
  TestModelGraph TestModelGraphND \
  TestEvalGraph TestEvalGraphND \
  TestOnlineModelBuilder TestOnlineModelBuilderND \
  TestOfflineModelBuilder TestOfflineModelBuilderND

noinst_PROGRAMS = \
  TestPlainHEX TestPlainHEXND

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
  $(top_builddir)/src/dlvhex/libdlvhexbase.la $(top_builddir)/src/dlvhex/libaspsolver.la \
  $(EXTSOLVER_LDFLAGS) $(EXTSOLVER_LIBS) \
  @LIBLTDL@ @LIBADD_DL@ @LIBCURL@
TestTestPluginStatic_CPPFLAGS = $(EXTSOLVER_CPPFLAGS)

# full debugging output

PLAINHEXSOURCES = \
	$(top_srcdir)/src/dlvhex/AnswerSet.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolverManager.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/BaseModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
  $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp \
	$(top_srcdir)/src/dlvhex/ComponentGraph.cpp \
	$(top_srcdir)/src/dlvhex/Configuration.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/DLVProcess.cpp \
	$(top_srcdir)/src/dlvhex/DLVresultParserDriver.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp \
	$(top_srcdir)/src/dlvhex/EvalGraphBuilder.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicEasy.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicOldDlvhex.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicTrivial.cpp \
	$(top_srcdir)/src/dlvhex/GuessAndCheckModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
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

TestPlainHEX_SOURCES = \
	$(top_srcdir)/testsuite/TestPlainHEX.cpp \
  $(PLAINHEXSOURCES)
TestPlainHEX_LDADD = $(EXTSOLVER_LDFLAGS) $(EXTSOLVER_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@
TestPlainHEX_CPPFLAGS = $(EXTSOLVER_CPPFLAGS)

TestEvalEndToEnd_SOURCES = \
	$(top_srcdir)/testsuite/TestEvalEndToEnd.cpp \
	$(top_srcdir)/src/dlvhex/BaseModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/PlainModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/WellfoundedModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/GuessAndCheckModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicOldDlvhex.cpp \
	$(top_srcdir)/src/dlvhex/EvalGraphBuilder.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/ComponentGraph.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/PredicateMask.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/DLVresultParserDriver.cpp \
	$(top_srcdir)/src/dlvhex/AnswerSet.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/InputProvider.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolverManager.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/DLVProcess.cpp \
	$(top_srcdir)/src/dlvhex/Configuration.cpp \
	$(top_srcdir)/src/dlvhex/ProcessBuf.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Printer.cpp \
	$(top_srcdir)/src/dlvhex/Registry.cpp \
	$(top_srcdir)/src/dlvhex/URLBuf.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestEvalEndToEnd_LDADD = $(EXTSOLVER_LDFLAGS) $(EXTSOLVER_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@
TestEvalEndToEnd_CPPFLAGS = $(EXTSOLVER_CPPFLAGS)

TestASPSolver_SOURCES = \
	$(top_srcdir)/testsuite/TestASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/DLVresultParserDriver.cpp \
	$(top_srcdir)/src/dlvhex/AnswerSet.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/InputProvider.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolverManager.cpp \
	$(top_srcdir)/src/dlvhex/DLVProcess.cpp \
	$(top_srcdir)/src/dlvhex/Configuration.cpp \
	$(top_srcdir)/src/dlvhex/PredicateMask.cpp \
	$(top_srcdir)/src/dlvhex/ProcessBuf.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Printer.cpp \
	$(top_srcdir)/src/dlvhex/Registry.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/URLBuf.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestASPSolver_LDADD = $(EXTSOLVER_LDFLAGS) $(EXTSOLVER_LIBS) @LIBLTDL@ @LIBADD_DL@ @LIBCURL@
TestASPSolver_CPPFLAGS = $(EXTSOLVER_CPPFLAGS) 

#TODO evalheuristic should (and could) be tested with fake model generator and fake interpretation
TestEvalHeuristic_SOURCES = \
	$(top_srcdir)/testsuite/TestEvalHeuristic.cpp \
	$(top_srcdir)/src/dlvhex/BaseModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/PlainModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/WellfoundedModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/GuessAndCheckModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicOldDlvhex.cpp \
	$(top_srcdir)/src/dlvhex/EvalGraphBuilder.cpp \
	$(top_srcdir)/src/dlvhex/ComponentGraph.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/PredicateMask.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/InputProvider.cpp \
	$(top_srcdir)/src/dlvhex/DLVresultParserDriver.cpp \
	$(top_srcdir)/src/dlvhex/AnswerSet.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolverManager.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/DLVProcess.cpp \
	$(top_srcdir)/src/dlvhex/Configuration.cpp \
	$(top_srcdir)/src/dlvhex/ProcessBuf.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Printer.cpp \
	$(top_srcdir)/src/dlvhex/Registry.cpp \
	$(top_srcdir)/src/dlvhex/URLBuf.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestEvalHeuristic_LDADD = $(EXTSOLVER_LDFLAGS) $(EXTSOLVER_LIBS) @LIBLTDL@ @LIBADD_DL@  @LIBCURL@
TestEvalHeuristic_CPPFLAGS = $(EXTSOLVER_CPPFLAGS) 

TestComponentGraph_SOURCES = \
	$(top_srcdir)/src/dlvhex/ComponentGraph.cpp \
	$(top_srcdir)/testsuite/TestComponentGraph.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/InputProvider.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Configuration.cpp \
	$(top_srcdir)/src/dlvhex/PredicateMask.cpp \
	$(top_srcdir)/src/dlvhex/Printer.cpp \
	$(top_srcdir)/src/dlvhex/Registry.cpp \
	$(top_srcdir)/src/dlvhex/URLBuf.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestComponentGraph_LDADD = @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

TestDependencyGraph_SOURCES = \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraphFull.cpp \
	$(top_srcdir)/testsuite/TestDependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/InputProvider.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/PredicateMask.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Printer.cpp \
	$(top_srcdir)/src/dlvhex/Registry.cpp \
	$(top_srcdir)/src/dlvhex/Configuration.cpp \
	$(top_srcdir)/src/dlvhex/URLBuf.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestDependencyGraph_LDADD = @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

TestHexParser_SOURCES = \
	$(top_srcdir)/testsuite/TestHexParser.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/InputProvider.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/PredicateMask.cpp \
	$(top_srcdir)/src/dlvhex/Printer.cpp \
	$(top_srcdir)/src/dlvhex/Configuration.cpp \
	$(top_srcdir)/src/dlvhex/Registry.cpp \
	$(top_srcdir)/src/dlvhex/URLBuf.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestHexParser_LDADD = @LIBLTDL@ @LIBADD_DL@ @LIBCURL@

TestTables_SOURCES = TestTables.cpp  $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/ID.cpp
TestTables_LDADD = @LIBLTDL@ @LIBADD_DL@ 

TestModelGraph_SOURCES = TestModelGraph.cpp dummytypes.cpp fixtureE2M2.cpp fixtureE2.cpp $(top_srcdir)/src/dlvhex/Logger.cpp

TestEvalGraph_SOURCES = TestEvalGraph.cpp dummytypes.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp

TestOnlineModelBuilder_SOURCES = TestOnlineModelBuilder.cpp dummytypes.cpp fixtureE1.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp

TestOfflineModelBuilder_SOURCES = TestOfflineModelBuilder.cpp dummytypes.cpp fixtureE1.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp

# test it with NDEBUG (the real situation)
TestPlainHEXND_SOURCES = $(TestPlainHEX_SOURCES)
TestPlainHEXND_LDADD = $(TestPlainHEX_LDADD)
TestPlainHEXND_CPPFLAGS = $(TestPlainHEX_CPPFLAGS) -DNDEBUG

TestEvalEndToEndND_SOURCES = $(TestEvalEndToEnd_SOURCES)
TestEvalEndToEndND_LDADD = $(TestEvalEndToEnd_LDADD)
TestEvalEndToEndND_CPPFLAGS = $(TestEvalEndToEnd_CPPFLAGS) -DNDEBUG

TestASPSolverND_SOURCES = $(TestASPSolver_SOURCES)
TestASPSolverND_LDADD = $(TestASPSolver_LDADD)
TestASPSolverND_CPPFLAGS = $(TestASPSolver_CPPFLAGS) -DNDEBUG

TestEvalHeuristicND_SOURCES = $(TestEvalHeuristic_SOURCES)
TestEvalHeuristicND_LDADD = $(TestEvalHeuristic_LDADD)
TestEvalHeuristicND_CPPFLAGS = $(TestEvalHeuristic_CPPFLAGS) -DNDEBUG

TestComponentGraphND_SOURCES = $(TestComponentGraph_SOURCES)
TestComponentGraphND_LDADD = $(TestComponentGraph_LDADD)
TestComponentGraphND_CPPFLAGS = $(TestComponentGraph_CPPFLAGS) -DNDEBUG

TestDependencyGraphND_SOURCES = $(TestDependencyGraph_SOURCES)
TestDependencyGraphND_LDADD = $(TestDependencyGraph_LDADD)
TestDependencyGraphND_CPPFLAGS = $(TestDependencyGraph_CPPFLAGS) -DNDEBUG

TestHexParserND_SOURCES = $(TestHexParser_SOURCES)
TestHexParserND_LDADD = $(TestHexParser_LDADD)
TestHexParserND_CPPFLAGS = $(TestHexParser_CPPFLAGS) -DNDEBUG

TestTablesND_SOURCES = $(TestTables_SOURCES)
TestTablesND_LDADD = $(TestTables_LDADD)
TestTablesND_CPPFLAGS = $(TestTables_CPPFLAGS) -DNDEBUG

TestModelGraphND_SOURCES = $(TestModelGraph_SOURCES)
TestModelGraphND_CPPFLAGS = $(TestModelGraph_CPPFLAGS) -DNDEBUG

TestEvalGraphND_SOURCES = $(TestEvalGraph_SOURCES)
TestEvalGraphND_CPPFLAGS = $(TestEvalGraph_CPPFLAGS) -DNDEBUG

TestOnlineModelBuilderND_SOURCES = $(TestOnlineModelBuilder_SOURCES)
TestOnlineModelBuilderND_CPPFLAGS = $(TestOnlineModelBuilder_CPPFLAGS) -DNDEBUG

TestOfflineModelBuilderND_SOURCES = $(TestOfflineModelBuilder_SOURCES)
TestOfflineModelBuilderND_CPPFLAGS = $(TestOfflineModelBuilder_CPPFLAGS) -DNDEBUG
