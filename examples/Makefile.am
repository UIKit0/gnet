## Process this file with automake to produce Makefile.in

@SET_MAKE@

noinst_PROGRAMS = echoserver echoserver-async echoserver-gserver \
		  echoclient echoclient-async echoclient-gconn	 \
		  echoserver-udp 				 \
		  echoclient-udp	 			 \
		  echoserver-unix 				 \
		  echoclient-unix 				 \
		  hash hfetch sdr
CLEANFILES     = $(EXTRA_PROGRAMS)

AM_CFLAGS = $(GLIB_CFLAGS) $(GTHREAD_CFLAGS)
INCLUDES = -I$(top_srcdir)/src 
LDADD    = $(top_builddir)/src/libgnet.la

##########

echoserver_SOURCES      	= echoserver.c
echoserver_async_SOURCES      	= echoserver-async.c
echoserver_gserver_SOURCES     	= echoserver-gserver.c

echoclient_SOURCES      	= echoclient.c
echoclient_async_SOURCES 	= echoclient-async.c
echoclient_gconn_SOURCES 	= echoclient-gconn.c

echoserver_udp_SOURCES  	= echoserver-udp.c

echoclient_udp_SOURCES  	= echoclient-udp.c

echoserver_unix_SOURCES 	= echoserver-unix.c

echoclient_unix_SOURCES 	= echoclient-unix.c

sdr_SOURCES 			= sdr.c
hash_SOURCES 			= hash.c
hfetch_SOURCES 			= hfetch.c
