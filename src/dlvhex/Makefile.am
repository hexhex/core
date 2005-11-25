AUTOMAKE_OPTIONS = foreign

bin_PROGRAMS = dlvhex

pkglib_LTLIBRARIES = libtypes.la


dlvhex_SOURCES = \
                parser.ypp \
                scanner.lpp \
                dlvhex.cpp \
                DependencyGraph.cpp \
                GraphBuilder.cpp \
                Component.cpp \
                FixpointModelGenerator.cpp \
                OrdinaryModelGenerator.cpp \
                GraphProcessor.cpp \
                ProgramBuilder.cpp

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
dlvhex_LDADD = libtypes.la libaspsolver.la # libplugin.la

#
# bison flags
#
AM_YFLAGS = -y -d

#
# make convenience lib
#
noinst_LTLIBRARIES = \
                     libaspsolver.la

libtypes_la_SOURCES = \
                      Term.cpp \
                      Atom.cpp \
                      Literal.cpp \
                      Interpretation.cpp \
                      PluginContainer.cpp \
                      PluginInterface.cpp \
                      Rule.cpp \
                      ExternalAtom.cpp \
                      helper.cpp

#libplugin_la_SOURCES = \
#                      PluginInterface.h

libaspsolver_la_SOURCES = \
                      globals.cpp \
                      ASPsolver.cpp \
                      DLVresultParser.ypp \
                      DLVresultScanner.lpp

libaspsolver_la_LIBADD = libtypes.la
