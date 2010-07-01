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

# install those libraries into $libdir
lib_LTLIBRARIES = libdlvhexbase.la libaspsolver.la

# the dlvhex binary
dlvhex_SOURCES = \
    dlvhex.cpp \
    RuleMLOutputBuilder.cpp 

#
# -static: statically link libdlvhexbase
# -export-dynamic: export all dynamic symbols, otherwise the plugins would use incompatible exceptions
#
#dlvhex_LDFLAGS = -static -export-dynamic
dlvhex_LDFLAGS = -export-dynamic

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
    Benchmarking.cpp \
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


# for more information on `-version-info current[:revision[:age]]' see
# <http://www.gnu.org/software/libtool/manual/libtool.html#Versioning>,
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
libdlvhexbase_la_LDFLAGS = -version-info 6:0:1
libaspsolver_la_LDFLAGS = -version-info 1:1:0

libdlvhexbase_la_LIBADD = @LIBLTDL@ @LIBADD_DL@

