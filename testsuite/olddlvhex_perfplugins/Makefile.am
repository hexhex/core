lib_LTLIBRARIES = libdlvhex_testplugin.la

# quick and dirty hack, build with installed dlvhex
AM_CPPFLAGS = \
    -I/home/staff/ps/include/ \
    $(BOOST_CPPFLAGS)

libdlvhex_testplugin_la_SOURCES = testplugin.cpp
libdlvhex_testplugin_la_LDFLAGS = -avoid-version -module
