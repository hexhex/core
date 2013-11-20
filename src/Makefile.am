# global preprocessor flags
AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_builddir)/src \
    -I$(top_srcdir)/src \
    $(BOOST_CPPFLAGS) \
    $(BM_CPPFLAGS) \
    $(DLVHEX_CFLAGS) \
    $(EXTSOLVER_CPPFLAGS) \
    @INCLTDL@ \
    @LIBCURL_CPPFLAGS@


# install dlvhex into $prefix/bin
bin_PROGRAMS = dlvhex2

# install those libraries into $libdir
lib_LTLIBRARIES = \
  libdlvhex2-base.la \
  libdlvhex2-aspsolver.la \
  libdlvhex2-mlpsolver.la \
  libdlvhex2-internalplugins.la

# the dlvhex binary
dlvhex2_SOURCES = dlvhex.cpp

#
# -static: statically link libdlvhexbase
# -export-dynamic: export all dynamic symbols, otherwise the plugins would use incompatible exceptions
#
#dlvhex2_LDFLAGS = -static -export-dynamic
dlvhex2_LDFLAGS = -export-dynamic $(EXTSOLVER_LDFLAGS) $(BOOST_THREAD_LDFLAGS)

#
# dlvhex itself also uses the convenience libs (see below)
#
dlvhex2_LDADD = \
  libdlvhex2-base.la libdlvhex2-mlpsolver.la \
  libdlvhex2-aspsolver.la libdlvhex2-internalplugins.la \
  $(BOOST_THREAD_LIBS) \
  @LIBLTDL@ @LIBADD_DL@ @LIBCURL@


# dlvhex API
libdlvhex2_base_la_SOURCES = \
    AnswerSet.cpp \
    AnnotatedGroundProgram.cpp \
    AnswerSetPrinterCallback.cpp \
    ASPSolverManager.cpp \
    AsynchronousModelGenerator.cpp \
    Atoms.cpp \
    BaseModelGenerator.cpp \
    Benchmarking.cpp \
    CAUAlgorithms.cpp \
    CDNLSolver.cpp \
    ClaspSolver.cpp \
    ComfortPluginInterface.cpp \
    ComponentGraph.cpp \
    Configuration.cpp \
    DependencyGraph.cpp \
    DumpingEvalGraphBuilder.cpp \
    Error.cpp \
    EvalGraphBuilder.cpp \
    EvalHeuristic.cpp \
    EvalHeuristicEasy.cpp \
    EvalHeuristicFromFile.cpp \
    EvalHeuristicGreedy.cpp \
    EvalHeuristicMonolithic.cpp \
    EvalHeuristicOldDlvhex.cpp \
    EvalHeuristicShared.cpp \
    EvalHeuristicTrivial.cpp \
    ExternalAtomEvaluationHeuristics.cpp \
    ExternalLearningHelper.cpp \
    ExtSourceProperties.cpp \
    FLPModelGeneratorBase.cpp \
    FunctionPlugin.cpp \
    GraphvizHelpers.cpp \
    PlainAuxPrinter.cpp \
    PlainModelGenerator.cpp \
    GenuinePlainModelGenerator.cpp \
    HTPlainModelGenerator.cpp \
    PredicateMask.cpp \
    WellfoundedModelGenerator.cpp \
    GenuineWellfoundedModelGenerator.cpp \
    GuessAndCheckModelGenerator.cpp \
    GenuineGuessAndCheckModelGenerator.cpp \
    GenuineGuessAndCheckModelGeneratorAsync.cpp \
    GenuineSolver.cpp \
    GringoGrounder.cpp \
    HexGrammar.cpp \
    HexParser.cpp \
    ID.cpp \
    InternalGroundASPSolver.cpp \
    InternalGroundDASPSolver.cpp \
    InternalGrounder.cpp \
    InputProvider.cpp \
    Interpretation.cpp \
    LiberalSafetyChecker.cpp \
    Logger.cpp \
    MLPSolver.cpp \
    MLPSyntaxChecker.cpp \
    Nogood.cpp \
    NogoodGrounder.cpp \
    PluginContainer.cpp \
    PluginInterface.cpp \
    Printer.cpp \
    ProgramCtx.cpp \
    Registry.cpp \
    SafetyChecker.cpp \
    SATSolver.cpp \
    State.cpp \
    Term.cpp \
    URLBuf.cpp \
    UnfoundedSetCheckHeuristics.cpp \
    UnfoundedSetChecker.cpp

# dlvhex MLP component
libdlvhex2_mlpsolver_la_SOURCES = \
	empty.cpp

# dlvhex ASP component
libdlvhex2_aspsolver_la_SOURCES = \
    ASPSolver.cpp \
    ASPSolver_dlv.cpp \
    ASPSolver_libclingo.cpp \
    DLVProcess.cpp \
    EvalHeuristicASP.cpp \
    DLVresultParserDriver.cpp \
    ProcessBuf.cpp 

# dlvhex internal plugins
libdlvhex2_internalplugins_la_SOURCES = \
    AggregatePlugin.cpp \
    ExistsPlugin.cpp \
    HigherOrderPlugin.cpp \
    ManualEvalHeuristicsPlugin.cpp \
    QueryPlugin.cpp \
    StrongNegationPlugin.cpp \
    WeakConstraintPlugin.cpp

# for more information on `-version-info current[:revision[:age]]' see
# <http://www.gnu.org/software/libtool/manual/libtool.html#Versioning>,
# So, libtool library versions are described by three integers:
# current
#     The most recent interface number that this library implements.
# revision
#     The implementation number of the current interface.
# age
#     The difference between the newest and oldest interfaces that this
#     library implements. In other words, the library implements all the
#     interface numbers in the range from number current - age to current. 
# If two libraries have identical current and age numbers, then the dynamic
# linker chooses the library with the greater revision number. 
#
# the USENIX paper `Library Interface Versioning in Solaris and Linux'
# <http://www.usenix.org/publications/library/proceedings/als00/2000papers/papers/full_papers/browndavid/browndavid.pdf>,
# and for library incompatibilities, see
# <http://www.dwheeler.com/program-library/Program-Library-HOWTO/x36.html#AEN137>.
#
# To quote the libtool manual:
#
# Here are a set of rules to help you update your library version
# information:
#
#   1. Start with version information of ‘0:0:0’ for each libtool
#   library.
#
#   2. Update the version information only immediately before a public
#   release of your software. More frequent updates are unnecessary,
#   and only guarantee that the current interface number gets larger
#   faster.
#
#   3. If the library source code has changed at all since the last
#   update, then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
#
#   4. If any interfaces have been added, removed, or changed since
#   the last update, increment current, and set revision to 0.
#
#   5. If any interfaces have been added since the last public
#   release, then increment age.
#
#   6. If any interfaces have been removed or changed since the last
#   public release, then set age to 0.
#
# Never try to set the interface numbers so that they correspond to
# the release number of your package. This is an abuse that only
# fosters misunderstanding of the purpose of library
# versions. Instead, use the -release flag (see Release numbers), but
# be warned that every release of your package will not be binary
# compatible with any other release.
#
# The following explanation may help to understand the above rules a
# bit better: consider that there are three possible kinds of
# reactions from users of your library to changes in a shared library:
#
#   1. Programs using the previous version may use the new version as
#   drop-in replacement, and programs using the new version can also
#   work with the previous one. In other words, no recompiling nor
#   relinking is needed. In this case, bump revision only, don't touch
#   current nor age.
#
#   2. Programs using the previous version may use the new version as
#   drop-in replacement, but programs using the new version may use
#   APIs not present in the previous one. In other words, a program
#   linking against the new version may fail with “unresolved symbols”
#   if linking against the old version at runtime: set revision to 0,
#   bump current and age.
#
#   3. Programs may need to be changed, recompiled, relinked in order
#   to use the new version. Bump current, set revision and age to 0.
#
libdlvhex2_base_la_LDFLAGS = -version-info 9:0:0 -export-dynamic $(EXTSOLVER_LDFLAGS)
libdlvhex2_mlpsolver_la_LDFLAGS = -version-info 2:0:1
libdlvhex2_aspsolver_la_LDFLAGS = -version-info 5:0:0
libdlvhex2_internalplugins_la_LDFLAGS = -version-info 3:0:0

libdlvhex2_base_la_LIBADD = $(EXTSOLVER_LIBADD) @LIBLTDL@ @LIBADD_DL@

