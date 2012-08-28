clean-local:
	if test x$(DO_BUILDCLASPGRINGO) == xyes; then \
		rm -rf clasp ; \
		rm -rf gringo ;\
	fi

all-local:
	if test x$(DO_BUILDCLASPGRINGO) == xyes; then \
    TOP_SRCDIR=$(top_srcdir) \
		BOOST_ROOT=$(NESTED_BOOSTROOT) \
    CXX=$(CXX) \
    CC=$(CC) \
    $(SHELL) $(top_srcdir)/checkout_build_potassco.sh ; \
	fi


EXTRA_DIST = gringo.patch
