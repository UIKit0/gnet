## Process this file with automake to produce Makefile.in

@SET_MAKE@

INCLUDES = -I$(top_srcdir)/src $(GLIB_CFLAGS)
LDADD = $(GLIB_LIBS) $(GTHREAD_LIBS)

TESTS = hash pack unpack url_test
EXTRA_PROGRAMS = $(TESTS) hostinfo dnstest
CLEANFILES = $(TESTS)


#####

dnstest_LDADD	  = $(top_builddir)/src/libgnet.la
hash_LDADD	  = $(top_builddir)/src/libgnet.la
hostinfo_LDADD	  = $(top_builddir)/src/libgnet.la
pack_LDADD	  = $(top_builddir)/src/libgnet.la
unpack_LDADD	  = $(top_builddir)/src/libgnet.la
url_test_SOURCES  = url_test.c
url_test_LDADD    = $(top_builddir)/src/libgnet.la

tests: $(EXTRA_PROGRAMS)
.PHONY: tests
