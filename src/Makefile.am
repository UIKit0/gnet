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
	udp.c			\
	iochannel.c		\
	md5.c			\
	sha.c			\
	pack.c			\
	url.c			\
	conn.c			\
	server.c

gnetinclude_HEADERS = 		\
	gnet.h			\
        inetaddr.h              \
        mcast.h           	\
	tcp.h			\
        udp.h			\
	iochannel.h		\
	md5.h			\
	sha.h			\
	pack.h			\
	url.h			\
	conn.h			\
	server.h

#EXTRA_PROGRAMS = pack unpack

#pack_SOURCES =		\
#	pack.c		\
#	packtest.c

#unpack_SOURCES =	\
#	pack.c		\
#	unpack.c

EXTRA_DIST = gnet-private.h
