## Process this file with automake to produce Makefile.in

@SET_MAKE@

noinst_PROGRAMS = echoclient       echoserver 			\
		  echoclient-async echoserver-async 		\
		  echoclient-gconn echoserver-gserver 		\
		  echoclient-udp   echoserver-udp 		\
		  echoclient-unix  echoserver-unix 		\
		  dnslookup hash hfetch hostinfo sdr
CLEANFILES     = $(EXTRA_PROGRAMS)

AM_CFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)
INCLUDES  = -I$(top_srcdir)/src 
LDADD     = $(top_builddir)/src/libgnet-$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION).la

##########

echoclient_SOURCES      	= echoclient.c
echoserver_SOURCES      	= echoserver.c

echoclient_async_SOURCES 	= echoclient-async.c
echoserver_async_SOURCES      	= echoserver-async.c

echoclient_gconn_SOURCES 	= echoclient-gconn.c
echoserver_gserver_SOURCES     	= echoserver-gserver.c

echoclient_udp_SOURCES  	= echoclient-udp.c
echoserver_udp_SOURCES  	= echoserver-udp.c

echoclient_unix_SOURCES 	= echoclient-unix.c
echoserver_unix_SOURCES 	= echoserver-unix.c

dnslookup_SOURCES      		= dnslookup.c
hash_SOURCES 			= hash.c
hfetch_SOURCES 			= hfetch.c
sdr_SOURCES 			= sdr.c
