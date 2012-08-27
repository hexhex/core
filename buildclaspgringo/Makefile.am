clean-local:
	if test x$(DO_BUILDCLASPGRINGO) == xyes; then \
		rm -rf clasp || \
		rm -rf gringo || \
		/bin/true \
	fi

all-local:
	if test x$(DO_BUILDCLASPGRINGO) == xyes; then \
		$(SHELL) $(top_srcdir)/checkout_build_potassco.sh ; \
	fi

