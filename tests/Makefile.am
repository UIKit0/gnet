## Process this file with automake to produce Makefile.in

@SET_MAKE@

TESTS = conn_http_test inetaddr_test ipv6_test tcp_test $(NETWORK_TESTS)
check_PROGRAMS = conn_http_test inetaddr_test ipv6_test tcp_test
check_SCRIPTS= $(NETWORK_TESTS)
#EXTRA_PROGRAMS = conn_http_test inetaddr_test ipv6_test
EXTRA_SCRIPTS = client_server_test.pl dns_test.pl
CLEANFILES = $(check_PROGRAMS) .client*out .server*out .client*diff .server*diff

INCLUDES = -I$(top_srcdir)/src $(GLIB_CFLAGS)
LDADD = $(GLIB_LIBS) $(GTHREAD_LIBS) $(top_builddir)/src/libgnet-$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION).la

conn_http_test_SOURCES= conn_http_test.c
inetaddr_test_SOURCES = inetaddr_test.c
ipv6_test_SOURCES     = ipv6_test.c
tcp_test_SOURCES      = tcp_test.c

if HAVE_CHECK
SUBDIRS_CHECK = check
else
SUBDIRS_CHECK =
endif

SUBDIRS = $(SUBDIRS_CHECK)

DIST_SUBDIRS = check

