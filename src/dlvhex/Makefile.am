AUTOMAKE_OPTIONS = foreign

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
                OrdinaryModelGenerator.cpp \
		OutputBuilder.cpp \
		parser.ypp \
                ProgramBuilder.cpp \
                ResultContainer.cpp \
                SafetyChecker.cpp \
                scanner.lpp

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
# bison flags
#
AM_YFLAGS = -y -d

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
                      GeneralError.cpp \
                      globals.cpp \
                      helper.cpp \
                      Literal.cpp \
                      PluginContainer.cpp \
                      PluginInterface.cpp \
	              Registry.cpp \
                      Rule.cpp \
                      Term.cpp

#libplugin_la_SOURCES = \
#                      PluginInterface.h

libaspsolver_la_SOURCES = \
                      ASPsolver.cpp \
                      DLVresultParser.ypp \
                      DLVresultScanner.lpp

libaspsolver_la_LIBADD = libdlvhexbase.la
