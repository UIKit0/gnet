## Process this file with automake to produce Makefile.in

@SET_MAKE@

EXTRA_DIST = makefile.mingw

INCLUDES = \
	-I$(top_srcdir)/src \
	$(GLIB_CFLAGS)

# Automatic tests
TESTS = url_test

# Manual tests
noinst_PROGRAMS = 	\
	dnstest		\
	hash		\
	hostinfo	\
	pack		\
	echoclient-socks\
	unpack		\
	urlparse	\
	$(TESTS)

#####

url_test_SOURCES  = url_test.c
url_test_LDADD    = $(top_builddir)/src/libgnet.la
 
dnstest_LDADD	  = $(top_builddir)/src/libgnet.la
hash_LDADD	  = $(top_builddir)/src/libgnet.la
hostinfo_LDADD	  = $(top_builddir)/src/libgnet.la
pack_LDADD	  = $(top_builddir)/src/libgnet.la
echoclient_socks_LDADD = $(top_builddir)/src/libgnet.la
unpack_LDADD	  = $(top_builddir)/src/libgnet.la
urlparse_LDADD	  = $(top_builddir)/src/libgnet.la

