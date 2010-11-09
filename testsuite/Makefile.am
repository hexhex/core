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
  TestEvalHeuristic TestEvalHeuristicND \
  TestComponentGraph TestComponentGraphND \
  TestDependencyGraph TestDependencyGraphND \
  TestHexParser TestHexParserND \
  TestTables TestTablesND \
  TestModelGraph TestModelGraphND \
  TestEvalGraph TestEvalGraphND \
  TestOnlineModelBuilder TestOnlineModelBuilderND \
  TestOfflineModelBuilder TestOfflineModelBuilderND

TESTS_ENVIRONMENT = EXAMPLESDIR=$(top_srcdir)/examples BOOST_TEST_REPORT_LEVEL=detailed BOOST_TEST_LOG_LEVEL=all

#TEST_SUITE_LOG = testsuite.log

check_PROGRAMS = \
  TestTypes \
  TestEvalHeuristic TestEvalHeuristicND \
  TestComponentGraph TestComponentGraphND \
  TestDependencyGraph TestDependencyGraphND \
  TestHexParser TestHexParserND \
  TestTables TestTablesND \
  TestModelGraph TestModelGraphND \
  TestEvalGraph TestEvalGraphND \
  TestOnlineModelBuilder TestOnlineModelBuilderND \
  TestOfflineModelBuilder TestOfflineModelBuilderND


TestTypes_SOURCES = TestTypes.cpp 
TestTypes_LDADD = \
	@LIBLTDL@ @LIBADD_DL@ 

# test it with full debugging output

#TODO evalheuristic should (and could) be tested with fake model generator and fake interpretation
TestEvalHeuristic_SOURCES = \
	$(top_srcdir)/testsuite/TestEvalHeuristic.cpp \
	$(top_srcdir)/src/dlvhex/Interpretation.cpp \
	$(top_srcdir)/src/dlvhex/FinalModelGenerator.cpp \
	$(top_srcdir)/src/dlvhex/EvalHeuristicOldDlvhex.cpp \
	$(top_srcdir)/src/dlvhex/EvalGraphBuilder.cpp \
	$(top_srcdir)/src/dlvhex/ComponentGraph.cpp \
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
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
	$(top_srcdir)/src/dlvhex/DependencyGraph.cpp \
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
	$(top_srcdir)/src/dlvhex/PluginInterface.cpp \
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
	$(top_srcdir)/src/dlvhex/Logger.cpp \
	$(top_srcdir)/src/dlvhex/ID.cpp \
	$(top_srcdir)/src/dlvhex/ProgramCtx.cpp \
	$(top_srcdir)/src/dlvhex/Error.cpp
TestHexParser_LDADD = @LIBLTDL@ @LIBADD_DL@ 

TestTables_SOURCES = TestTables.cpp  $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/ID.cpp
TestTables_LDADD = @LIBLTDL@ @LIBADD_DL@ 

TestModelGraph_SOURCES = TestModelGraph.cpp dummytypes.cpp fixtureE2M2.cpp fixtureE2.cpp $(top_srcdir)/src/dlvhex/Logger.cpp

TestEvalGraph_SOURCES = TestEvalGraph.cpp dummytypes.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp

TestOnlineModelBuilder_SOURCES = TestOnlineModelBuilder.cpp dummytypes.cpp fixtureE1.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp

TestOfflineModelBuilder_SOURCES = TestOfflineModelBuilder.cpp dummytypes.cpp fixtureE1.cpp fixtureE2.cpp fixtureEx1.cpp $(top_srcdir)/src/dlvhex/Logger.cpp $(top_srcdir)/src/dlvhex/CAUAlgorithms.cpp

# test it with NDEBUG (the real situation)
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
