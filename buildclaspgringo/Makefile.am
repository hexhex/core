clean-local:
	if test x$(DO_BUILDCLASPGRINGO) == xyes; then \
		rm -rf clasp ; \
		rm -rf gringo ;\
	fi

all-local:
	if test x$(DO_BUILDCLASPGRINGO) == xyes; then \
    TOP_SRCDIR=$(top_srcdir) \
		BOOST_ROOT=$(NESTED_BOOSTROOT) \
    CXX="$(CXX) -fPIC" \
    CC="$(CC) -fPIC" \
    USING_CLANG=$(using_clang) \
    $(SHELL) $(top_srcdir)/build_potassco.sh ; \
	fi


EXTRA_DIST = gringo-clang.patch gringo.patch gringo-patch-cond.diff gringo-patch-domain-fwd-decl.diff gringo-patch-domain-fwd-decl-builtsource.diff gringo-patch-unpool-pred.diff

