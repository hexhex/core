lib_LTLIBRARIES = libldlvhexmcsdiagexpl.la
plugin_LTLIBRARIES = libdlvhexmcsdiagexpl.la

libldlvhexmcsdiagexpl_la_SOURCES = InputConverter.cpp \
                                   BridgeRule.cpp \
                                   BridgeRuleEntry.cpp \
                                   ParseContext.cpp \
                                   ACC_ContextAtom.cpp \
                                   DLV_ASP_ContextAtom.cpp \
                                   OutputRewriter.cpp \
                                   EquilibriumPrintVisitor.cpp \
                                   DiagExplPrintVisitor.cpp \
                                   Timing.cpp \
                                   Global.cpp
# for more information on `-version-info' see
# <http://www.gnu.org/software/libtool/manual.html#Versioning>, and
# the USENIX paper `Library Interface Versioning in Solaris and Linux'
# <http://www.usenix.org/publications/library/proceedings/als00/2000papers/papers/full_papers/browndavid/browndavid.pdf>
libldlvhexmcsdiagexpl_la_LDFLAGS = -version-info 1:0:0

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
        $(BOOST_CPPFLAGS) \
	$(DLVHEX_CFLAGS)

libdlvhexmcsdiagexpl_la_SOURCES = MCSdiagexplPlugin.cpp
libdlvhexmcsdiagexpl_la_LDFLAGS = -avoid-version -module
libdlvhexmcsdiagexpl_la_LIBADD = libldlvhexmcsdiagexpl.la

