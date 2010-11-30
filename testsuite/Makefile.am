SUBDIRS = benchgen olddlvhex_perfplugins

# global preprocessor flags
AM_CPPFLAGS = \
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


GRINGO_TRUNK_DIR=/home/staff/ps/Documents/clasp/potassco-sf/trunk/gringo
CLINGOLDADD = \
  -L$(GRINGO_TRUNK_DIR)/build/release/lib/ \
  -lappclingo -lclasp -lappgringo -lgringo -llua -lluasql -lprogram_opts -lsqlite3 \
  $(BOOST_FILESYSTEM_LDFLAGS) $(BOOST_FILESYSTEM_LIBS) $(BOOST_SYSTEM_LDFLAGS) $(BOOST_SYSTEM_LIBS) 
CLINGOCPPFLAGS= \
  -DHAVE_LIBCLINGO \
  -I$(GRINGO_TRUNK_DIR)/libprogram_opts/ \
  -I$(GRINGO_TRUNK_DIR)/libgringo/ \
  -I$(GRINGO_TRUNK_DIR)/libclasp/ \
  -I$(GRINGO_TRUNK_DIR)/app/

CLINGOLDADD= 
CLINGOCPPFLAGS= 

TESTS = \
  TestEvalEndToEnd TestEvalEndToEndND \
  TestEvalHeuristic TestEvalHeuristicND \
  TestComponentGraph TestComponentGraphND \
  TestDependencyGraph TestDependencyGraphND \
  TestHexParser TestHexParserND \
  TestHexParserModule TestHexParserModuleND \
  TestTables TestTablesND \
  TestModelGraph TestModelGraphND \
  TestEvalGraph TestEvalGraphND \
  TestOnlineModelBuilder TestOnlineModelBuilderND \
  TestOfflineModelBuilder TestOfflineModelBuilderND

TESTS_ENVIRONMENT = EXAMPLESDIR=$(top_srcdir)/examples BOOST_TEST_REPORT_LEVEL=detailed BOOST_TEST_LOG_LEVEL=all

#TEST_SUITE_LOG = testsuite.log

check_PROGRAMS = \
  TestASPSolver TestASPSolverND \
  TestEvalEndToEnd TestEvalEndToEndND \
  TestEvalHeuristic TestEvalHeuristicND \
  TestComponentGraph TestComponentGraphND \
  TestDependencyGraph TestDependencyGraphND \
  TestHexParser TestHexParserND \
  TestHexParserModule TestHexParserModuleND \
  TestTables TestTablesND \
  TestModelGraph TestModelGraphND \
  TestEvalGraph TestEvalGraphND \
  TestOnlineModelBuilder TestOnlineModelBuilderND \
  TestOfflineModelBuilder TestOfflineModelBuilderND

noinst_PROGRAMS = \
  TestMCSIE TestMCSIEND \
  TestPlainHEX TestPlainHEXND


# full debugging output

PLAINHEXSOURCES = \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/FinalModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicOldDlvhex.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicTrivial.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicEasy.cpp \
	$(top_srcdir)/src/dlvhex/EvalGraphBuilder.cpp \
  $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp \
	$(top_srcdir)/src/dlvhex/ComponentGraph.cpp \
	$(top_srcdir)/src/dlvhex/OutputBuilder.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/DLVresultParserDriver.cpp \
	$(top_srcdir)/src/dlvhex/AnswerSet.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolverManager.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/DLVProcess.cpp \
	$(top_srcdir)/src/dlvhex/globals.cpp \
	$(top_srcdir)/src/dlvhex/ProcessBuf.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp

TestMCSIE_SOURCES = \
	$(top_srcdir)/testsuite/TestMCSIE.cpp \
	$(top_srcdir)/testsuite/mcsie_perftests/src/DLV_ASP_ContextAtom.cpp \
	$(top_srcdir)/testsuite/mcsie_perftests/src/InputConverter.cpp \
	$(top_srcdir)/testsuite/mcsie_perftests/src/BridgeRule.cpp \
	$(top_srcdir)/testsuite/mcsie_perftests/src/BridgeRuleEntry.cpp \
	$(top_srcdir)/testsuite/mcsie_perftests/src/Global.cpp \
	$(top_srcdir)/testsuite/mcsie_perftests/src/Timing.cpp \
	$(top_srcdir)/testsuite/mcsie_perftests/src/ParseContext.cpp \
	$(top_srcdir)/testsuite/mcsie_perftests/src/OutputRewriter.cpp \
  $(PLAINHEXSOURCES)
TestMCSIE_LDADD = @LIBLTDL@ @LIBADD_DL@
TestMCSIE_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/testsuite/mcsie_perftests/include

TestPlainHEX_SOURCES = \
	$(top_srcdir)/testsuite/TestPlainHEX.cpp \
  $(PLAINHEXSOURCES)
TestPlainHEX_LDADD = $(CLINGOLDADD) @LIBLTDL@ @LIBADD_DL@
TestPlainHEX_CPPFLAGS = $(CLINGOCPPFLAGS) $(AM_CPPFLAGS)

TestEvalEndToEnd_SOURCES = \
	$(top_srcdir)/testsuite/TestEvalEndToEnd.cpp \
	$(top_srcdir)/src/dlvhex/FinalModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicOldDlvhex.cpp \
	$(top_srcdir)/src/dlvhex/EvalGraphBuilder.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/ComponentGraph.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/DLVresultParserDriver.cpp \
	$(top_srcdir)/src/dlvhex/AnswerSet.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolverManager.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/DLVProcess.cpp \
	$(top_srcdir)/src/dlvhex/globals.cpp \
	$(top_srcdir)/src/dlvhex/ProcessBuf.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestEvalEndToEnd_LDADD = @LIBLTDL@ @LIBADD_DL@ 

TestASPSolver_SOURCES = \
	$(top_srcdir)/testsuite/TestASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/DLVresultParserDriver.cpp \
	$(top_srcdir)/src/dlvhex/AnswerSet.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolverManager.cpp \
	$(top_srcdir)/src/dlvhex/DLVProcess.cpp \
	$(top_srcdir)/src/dlvhex/globals.cpp \
	$(top_srcdir)/src/dlvhex/ProcessBuf.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestASPSolver_LDADD = $(CLINGOLDADD) @LIBLTDL@ @LIBADD_DL@
TestASPSolver_CPPFLAGS = $(CLINGOCPPFLAGS) $(AM_CPPFLAGS) 

#TODO evalheuristic should (and could) be tested with fake model generator and fake interpretation
TestEvalHeuristic_SOURCES = \
	$(top_srcdir)/testsuite/TestEvalHeuristic.cpp \
	$(top_srcdir)/src/dlvhex/FinalModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicOldDlvhex.cpp \
	$(top_srcdir)/src/dlvhex/EvalGraphBuilder.cpp \
	$(top_srcdir)/src/dlvhex/ComponentGraph.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/DLVresultParserDriver.cpp \
	$(top_srcdir)/src/dlvhex/AnswerSet.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolverManager.cpp \
	$(top_srcdir)/src/dlvhex/ASPSolver.cpp \
	$(top_srcdir)/src/dlvhex/DLVProcess.cpp \
	$(top_srcdir)/src/dlvhex/globals.cpp \
	$(top_srcdir)/src/dlvhex/ProcessBuf.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestEvalHeuristic_LDADD = @LIBLTDL@ @LIBADD_DL@ 

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
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestComponentGraph_LDADD = @LIBLTDL@ @LIBADD_DL@ 

TestDependencyGraph_SOURCES = \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraphFull.cpp \
	$(top_srcdir)/testsuite/TestDependencyGraph.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/Benchmarking.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestDependencyGraph_LDADD = @LIBLTDL@ @LIBADD_DL@ 

TestHexParser_SOURCES = \
	$(top_srcdir)/testsuite/TestHexParser.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestHexParser_LDADD = @LIBLTDL@ @LIBADD_DL@ 

TestHexParserModule_SOURCES = \
	$(top_srcdir)/testsuite/TestHexParserModule.cpp \
	$(top_srcdir)/src/dlvhex/HexGrammarPTToASTConverter.cpp \
	$(top_srcdir)/src/dlvhex/HexParser.cpp \
	$(top_srcdir)/src/dlvhex/Atoms.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/ModuleSyntaxChecker.cpp \
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestHexParserModule_LDADD = @LIBLTDL@ @LIBADD_DL@ 

TestTables_SOURCES = TestTables.cpp  $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/ID.cpp
TestTables_LDADD = @LIBLTDL@ @LIBADD_DL@ 

TestModelGraph_SOURCES = TestModelGraph.cpp dummytypes.cpp fixtureE2M2.cpp fixtureE2.cpp $(top_srcdir)/src/dlvhex/Logger.cpp

TestEvalGraph_SOURCES = TestEvalGraph.cpp dummytypes.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp

TestOnlineModelBuilder_SOURCES = TestOnlineModelBuilder.cpp dummytypes.cpp fixtureE1.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp

TestOfflineModelBuilder_SOURCES = TestOfflineModelBuilder.cpp dummytypes.cpp fixtureE1.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp

# test it with NDEBUG (the real situation)
TestMCSIEND_SOURCES = $(TestMCSIE_SOURCES)
TestMCSIEND_LDADD = $(TestMCSIE_LDADD)
TestMCSIEND_CPPFLAGS = $(TestMCSIE_CPPFLAGS) -DNDEBUG

TestPlainHEXND_SOURCES = $(TestPlainHEX_SOURCES)
TestPlainHEXND_LDADD = $(TestPlainHEX_LDADD)
TestPlainHEXND_CPPFLAGS = $(TestPlainHEX_CPPFLAGS) -DNDEBUG

TestEvalEndToEndND_SOURCES = $(TestEvalEndToEnd_SOURCES)
TestEvalEndToEndND_LDADD = $(TestEvalEndToEnd_LDADD)
TestEvalEndToEndND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestASPSolverND_SOURCES = $(TestASPSolver_SOURCES)
TestASPSolverND_LDADD = $(TestASPSolver_LDADD)
TestASPSolverND_CPPFLAGS = $(TestASPSolver_CPPFLAGS) -DNDEBUG

TestEvalHeuristicND_SOURCES = $(TestEvalHeuristic_SOURCES)
TestEvalHeuristicND_LDADD = $(TestEvalHeuristic_LDADD)
TestEvalHeuristicND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestComponentGraphND_SOURCES = $(TestComponentGraph_SOURCES)
TestComponentGraphND_LDADD = $(TestComponentGraph_LDADD)
TestComponentGraphND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestDependencyGraphND_SOURCES = $(TestDependencyGraph_SOURCES)
TestDependencyGraphND_LDADD = $(TestDependencyGraph_LDADD)
TestDependencyGraphND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestHexParserND_SOURCES = $(TestHexParser_SOURCES)
TestHexParserND_LDADD = $(TestHexParser_LDADD)
TestHexParserND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestHexParserModuleND_SOURCES = $(TestHexParserModule_SOURCES)
TestHexParserModuleND_LDADD = $(TestHexParserModule_LDADD)
TestHexParserModuleND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestTablesND_SOURCES = $(TestTables_SOURCES)
TestTablesND_LDADD = $(TestTables_LDADD)
TestTablesND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestModelGraphND_SOURCES = $(TestModelGraph_SOURCES)
TestModelGraphND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestEvalGraphND_SOURCES = $(TestEvalGraph_SOURCES)
TestEvalGraphND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestOnlineModelBuilderND_SOURCES = $(TestOnlineModelBuilder_SOURCES)
TestOnlineModelBuilderND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG

TestOfflineModelBuilderND_SOURCES = $(TestOfflineModelBuilder_SOURCES)
TestOfflineModelBuilderND_CPPFLAGS = $(AM_CPPFLAGS) -DNDEBUG
