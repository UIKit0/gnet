## Process this file with automake to produce Makefile.in

@SET_MAKE@

INCLUDES = -DG_LOG_DOMAIN=\"GNet\" @GNET_DEBUG_FLAGS@ \
	$(GLIB_CFLAGS) $(GTHREAD_CFLAGS) 

gnetincludedir = $(includedir)/gnet-$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION)

lib_LTLIBRARIES = libgnet.la

libgnet_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE) \
	$(GLIB_LIBS) $(GTHREAD_LIBS) 

libgnet_la_SOURCES = 		\
	gnet.c			\
	gnet-private.c		\
	ipv6.c			\
	inetaddr.c		\
	mcast.c			\
	tcp.c			\
	unix.c			\
	udp.c			\
	iochannel.c		\
	socks.c			\
	socks-private.c		\
	md5.c			\
	sha.c			\
	pack.c			\
	uri.c			\
	conn.c			\
	server.c		\
	usagi_ifaddrs.c		\
	base64.c

gnetinclude_HEADERS = 		\
	gnet.h			\
	ipv6.h			\
        inetaddr.h              \
        mcast.h           	\
	tcp.h			\
	unix.h			\
        udp.h			\
	iochannel.h		\
	socks.h			\
	md5.h			\
	sha.h			\
	pack.h			\
	uri.h			\
	conn.h			\
	server.h		\
	base64.h
