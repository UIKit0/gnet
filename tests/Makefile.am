## Process this file with automake to produce Makefile.in

@SET_MAKE@

EXTRA_PROGRAMS = pack unpack hash dnstest hostinfo
CLEANFILES = pack unpack hash dnstest hostinfo

INCLUDES += `gnet-config --cflags`
LDFLAGS += `gnet-config --libs`

hash_SOURCES =		\
	hash.c

pack_SOURCES =		\
	pack.c

unpack_SOURCES =	\
	unpack.c

dnstest_SOURCES =	\
	dnstest.c

hostinfo_SOURCES = 	\
	hostinfo.c

tests:	pack unpack hash dnstest hostinfo
