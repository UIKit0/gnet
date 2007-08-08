## Process this file with automake to produce Makefile.in

@SET_MAKE@

noinst_LIBRARIES = libgnetxmlrpc.a

noinst_PROGRAMS = \
	test-xmlrpc-client \
	test-xmlrpc-server

AM_CFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)
INCLUDES = -I$(top_srcdir)/src 
LDADD = $(top_builddir)/src/libgnet-$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION).la

libgnetxmlrpc_a_SOURCES = xmlrpc-client.c xmlrpc-server.c xmlrpc.h
libgnetxmlrpc_a_LIBADD = $(top_builddir)/src/libgnet-$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION).la

test_xmlrpc_client_SOURCES = test-xmlrpc-client.c xmlrpc.h
test_xmlrpc_client_LDADD = \
	libgnetxmlrpc.a \
	$(top_builddir)/src/libgnet-$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION).la

test_xmlrpc_server_SOURCES = test-xmlrpc-server.c xmlrpc.h
test_xmlrpc_server_LDADD = \
	libgnetxmlrpc.a \
	$(top_builddir)/src/libgnet-$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION).la
