# run the test script
TESTS = run-dlvhex-tests.sh

EXTRA_DIST = run-dlvhex-tests.sh diagexpl_comp.py

# some variables for the test-script
DLVHEX_ABS_TOP_BUILDDIR=$(realpath $(top_builddir))
DLVHEX_ABS_TOP_SRCDIR=$(realpath $(top_srcdir))
TESTS_ENVIRONMENT = EXAMPLES=$(DLVHEX_ABS_TOP_SRCDIR)/examples DLVHEX=dlvhex PARAMETERS="-s --plugindir=$(DLVHEX_ABS_TOP_BUILDDIR)/src:$(DLVHEX_ABS_TOP_BUILDDIR)/examples"
