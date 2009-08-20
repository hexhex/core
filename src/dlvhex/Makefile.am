AUTOMAKE_OPTIONS = gnu

# global preprocessor flags
AM_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_builddir)/src \
    -I$(top_srcdir)/src \
    $(BOOST_CPPFLAGS) \
    @INCLTDL@ \
    @LIBCURL_CPPFLAGS@


# install dlvhex into $prefix/bin
bin_PROGRAMS = dlvhex

# install those libraries into $pkglibdir/lib
pkglib_LTLIBRARIES = libdlvhexbase.la libaspsolver.la

# the dlvhex binary
dlvhex_SOURCES = \
    dlvhex.cpp \
    RuleMLOutputBuilder.cpp 

#
# -static: statically link libdlvhexbase
# -export-dynamic: export all dynamic symbols, otherwise the plugins would use incompatible exceptions
#
dlvhex_LDFLAGS = -static -export-dynamic

#
# dlvhex itself also uses the convenience libs (see below)
#
dlvhex_LDADD = @LIBLTDL@ @LIBADD_DL@ @LIBCURL@ libdlvhexbase.la libaspsolver.la

# dlvhex API
libdlvhexbase_la_SOURCES = \
    Atom.cpp \
    AggregateAtom.cpp \
    AtomNode.cpp \
    AtomSet.cpp \
    BoostComponentFinder.cpp \
    Component.cpp \
    ComponentFinder.cpp \
    Dependency.cpp \
    DependencyGraph.cpp \
    EvaluateExtatom.cpp \
    ExternalAtom.cpp \
    Error.cpp \
    FixpointModelGenerator.cpp \
    globals.cpp \
    GraphBuilder.cpp \
    GraphProcessor.cpp \
    GuessCheckModelGenerator.cpp \
    Literal.cpp \
    NodeGraph.cpp \
    OrdinaryModelGenerator.cpp \
    PluginContainer.cpp \
    PluginInterface.cpp \
    PrintVisitor.cpp \
    Program.cpp \
    ProgramCtx.cpp \
    Repository.cpp \
    Registry.cpp \
    Rule.cpp \
    SafetyChecker.cpp \
    State.cpp \
    Term.cpp \
    ResultContainer.cpp \
    TextOutputBuilder.cpp \
    HexGrammarPTToASTConverter.cpp \
    HexParserDriver.cpp \
    AnswerSet.cpp \
    URLBuf.cpp

# dlvhex ASP component
libaspsolver_la_SOURCES = \
    DLVresultParserDriver.cpp \
    ASPSolver.cpp \
    DLVProcess.cpp \
    ProcessBuf.cpp


# for more information on `-version-info' see
# <http://www.gnu.org/software/libtool/manual.html#Versioning>, and
# the USENIX paper `Library Interface Versioning in Solaris and Linux'
# <http://www.usenix.org/publications/library/proceedings/als00/2000papers/papers/full_papers/browndavid/browndavid.pdf>
libdlvhexbase_la_LDFLAGS = -version-info 5:1:0
libaspsolver_la_LDFLAGS = -version-info 1:0:0

libdlvhexbase_la_LIBADD = @LIBLTDL@ @LIBADD_DL@

