## Process this file with automake to produce Makefile.in

@SET_MAKE@

INCLUDES = -I$(top_srcdir)/src $(GLIB_CFLAGS)
LDADD = $(GLIB_LIBS) $(GTHREAD_LIBS)

TESTS = hash pack unpack inetaddr_test uri_test ipv6_test
EXTRA_PROGRAMS = $(TESTS) hostinfo dnstest
CLEANFILES = $(TESTS) hostinfo dnstest


#####

dnstest_LDADD	      = $(top_builddir)/src/libgnet.la
hash_LDADD	      = $(top_builddir)/src/libgnet.la
hostinfo_LDADD	      = $(top_builddir)/src/libgnet.la
pack_LDADD	      = $(top_builddir)/src/libgnet.la
unpack_LDADD	      = $(top_builddir)/src/libgnet.la
inetaddr_test_LDADD   = $(top_builddir)/src/libgnet.la
ipv6_test_LDADD       = $(top_builddir)/src/libgnet.la
uri_test_LDADD        = $(top_builddir)/src/libgnet.la

inetaddr_test_SOURCES = inetaddr_test.c
ipv6_test_SOURCES     = ipv6_test.c
uri_test_SOURCES      = uri_test.c

tests: $(EXTRA_PROGRAMS)
.PHONY: tests
