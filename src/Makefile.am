## Process this file with automake to produce Makefile.in

@SET_MAKE@

INCLUDES = -DG_LOG_DOMAIN=\"GNet\" @GNET_DEBUG_FLAGS@

gnetincludedir = $(includedir)/gnet

lib_LTLIBRARIES = libgnet.la

libgnet_la_LDFLAGS = \
	-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
	-release $(LT_RELEASE)

libgnet_la_SOURCES = 		\
	gnet.c			\
	gnet-private.c		\
	inetaddr.c		\
	mcast.c			\
	tcp.c			\
	ftp.c			\
	udp.c

gnetinclude_HEADERS = 		\
	gnet.h			\
	gnet-private.h		\
        inetaddr.h              \
        mcast.h           	\
	tcp.h			\
	ftp.h			\
        udp.h
