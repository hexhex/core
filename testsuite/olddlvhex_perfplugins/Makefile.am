lib_LTLIBRARIES = libdlvhex_aboveplugin.la

# quick and dirty hack, build with installed dlvhex
AM_CPPFLAGS = \
    -I/home/staff/ps/include/ \
    $(BOOST_CPPFLAGS)

libdlvhex_aboveplugin_la_SOURCES = aboveplugin.cpp
libdlvhex_aboveplugin_la_LDFLAGS = -avoid-version -module
