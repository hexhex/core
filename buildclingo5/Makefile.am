clean-local:
	if test x$(DO_BUILDCLINGO5) == xyes; then \
		rm -rf clingo5 ; \
	fi

all-local:
	if test x$(DO_BUILDCLINGO5) == xyes; then \
		TOP_SRCDIR=$(top_srcdir) \
		BOOST_ROOT=$(NESTED_BOOSTROOT) \
		CXX="$(CXX)" USING_CLANG=$(using_clang) \
		$(SHELL) $(top_srcdir)/build_clingo5.sh ; \
	fi

distclean:
	-rm -rf Makefile

EXTRA_DIST = python.cc.patch SConstruct.patch SConstruct-lua.patch SConstruct-python.patch 
