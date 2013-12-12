clean-local:
	if test x$(DO_BUILDOWLCPP) == xyes; then \
		echo "" > /dev/null \
	fi

all-local:
	if test x$(DO_BUILDOWLCPP) == xyes; then \
		TOP_SRCDIR=$(top_srcdir) \
		CXX="$(CXX)" USING_CLANG=$(using_clang) \
		$(SHELL) $(top_srcdir)/checkout_build_owlcpp.sh $(OWLCPP_ROOT) $(BOOST_MAJOR_VERSION); \
	elif test x$(OWLCPP_ROOT) != x; then \
		TOP_SRCDIR=$(top_srcdir) \
		CXX="$(CXX)" USING_CLANG=$(using_clang) \
		$(SHELL) $(top_srcdir)/checkout_build_owlcpp.sh $(OWLCPP_ROOT) $(BOOST_MAJOR_VERSION); \
	fi

EXTRA_DIST = 

