## Process this file with automake to produce Makefile.in

@SET_MAKE@

TESTS = hash_test inetaddr_test ipv6_test pack_test unpack_test uri_test $(NETWORK_TESTS)
EXTRA_PROGRAMS = hash_test inetaddr_test ipv6_test pack_test unpack_test uri_test
CLEANFILES = $(EXTRA_PROGRAMS) .client*out .server*out

INCLUDES = -I$(top_srcdir)/src $(GLIB_CFLAGS)
LDADD = $(GLIB_LIBS) $(GTHREAD_LIBS)



#####

hash_test_LDADD	      = $(top_builddir)/src/libgnet.la
inetaddr_test_LDADD   = $(top_builddir)/src/libgnet.la
ipv6_test_LDADD       = $(top_builddir)/src/libgnet.la
pack_test_LDADD	      = $(top_builddir)/src/libgnet.la
unpack_test_LDADD     = $(top_builddir)/src/libgnet.la
uri_test_LDADD        = $(top_builddir)/src/libgnet.la

hash_test_SOURCES     = hash_test.c
inetaddr_test_SOURCES = inetaddr_test.c
uri_test_SOURCES      = uri_test.c
pack_test_SOURCES     = pack_test.c
unpack_test_SOURCES   = unpack_test.c
uri_test_SOURCES      = uri_test.c
