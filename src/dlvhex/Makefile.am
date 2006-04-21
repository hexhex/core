AUTOMAKE_OPTIONS = foreign

MAINTAINERCLEANFILES = \
                       HexParser.hpp \
                       HexParser.output \
                       location.hh \
                       position.hh \
                       stack.hh


bin_PROGRAMS = dlvhex

pkglib_LTLIBRARIES = libdlvhexbase.la


dlvhex_SOURCES = \
		AtomNode.cpp \
                BoostComponentFinder.cpp \
                Component.cpp \
                ComponentFinder.cpp \
                DependencyGraph.cpp \
                dlvhex.cpp \
                FixpointModelGenerator.cpp \
                GuessCheckModelGenerator.cpp \
                GraphBuilder.cpp \
                GraphProcessor.cpp \
		HexParser.ypp \
                OrdinaryModelGenerator.cpp \
		OutputBuilder.cpp \
		ParserDriver.cpp \
		HexParserDriver.cpp \
                ProgramBuilder.cpp \
                ResultContainer.cpp \
                SafetyChecker.cpp \
                HexScanner.lpp

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/src -I$(top_srcdir)/src

#
# -ldl is for dynamic linking against the external module libs
# -Wl,-rpath is specifying . to be the search path for shared libs
# (dlvhex should use libs it finds in its own directory)
#
dlvhex_LDFLAGS = -ldl -Wl,-rpath,.

#
# dlvhex itself also uses the convenience libs (see below)
#
dlvhex_LDADD = libdlvhexbase.la libaspsolver.la # libplugin.la

#
# don't let automake rename and move the output files of flex/bison,
# this doesn't work for pure C++ parser (yet)
#
.lpp.cpp:
	$(LEXCOMPILE) -o $@ $<
.ypp.cpp:
	$(YACCCOMPILE) -o $@ $<

#
# bison flags
#
#AM_YFLAGS = -y -d

#
# make convenience lib
#
noinst_LTLIBRARIES = \
                     libaspsolver.la

libdlvhexbase_la_SOURCES = \
                      Atom.cpp \
		      AtomFactory.cpp \
                      AtomSet.cpp \
                      ExternalAtom.cpp \
                      Error.cpp \
                      globals.cpp \
                      helper.cpp \
                      Literal.cpp \
                      PluginContainer.cpp \
                      PluginInterface.cpp \
                      Program.cpp \
	              Repository.cpp \
	              Registry.cpp \
                      Rule.cpp \
                      Term.cpp

#libplugin_la_SOURCES = \
#                      PluginInterface.h

libaspsolver_la_SOURCES = \
                      ASPsolver.cpp \
                      DLVresultParser.ypp \
                      DLVresultScanner.lpp \
		      DLVresultParserDriver.cpp \
		      ParserDriver.cpp

libaspsolver_la_LIBADD = libdlvhexbase.la
