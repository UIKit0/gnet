## Process this file with automake to produce Makefile.in

@SET_MAKE@

EXTRA_PROGRAMS = sdr jmsend jmreceive hfetch echoserver echoclient \
		 hostinfo dnstest ljoust pack unpack hash \
		 echoserver-udp echoclient-udp

CLEANFILES = sdr jmsend jmreceive hfetch echoserver echoclient \
	     hostinfo dnstest ljoust pack hash \
	     echoserver-udp echoclient-udp

INCLUDES += `gnet-config --cflags`
LDFLAGS += `gnet-config --libs`


##########
# Main examples

sdr_SOURCES = \
	sdr.c

hfetch_SOURCES = 	\
	hfetch.c

echoserver_SOURCES = 	\
	echoserver.c

echoclient_SOURCES =	\
	echoclient.c

echoserver_udp_SOURCES = \
	echoserver-udp.c

echoclient_udp_SOURCES = \
	echoclient-udp.c

dnstest_SOURCES =	\
	dnstest.c

hash_SOURCES =		\
	hash.c

pack_SOURCES =		\
	pack.c

unpack_SOURCES =	\
	unpack.c


##########
# jmtransfer examples

jmsend_SOURCES = 	\
	jmsend.c 	\
	jmtransfer.c	\
	jmtransfer.h

jmreceive_SOURCES = 	\
	jmreceive.c	\
	jmtransfer.c	\
	jmtransfer.h


##########
# ljoust requires gtk

ljoust:
	$(CC) ljoust.c -o ljoust `gtk-config --cflags --libs` `gnet-config --cflags --libs`


##########
# hostinfo is out-of-date

hostinfo_SOURCES = 	\
	hostinfo.c


########################################

examples: echoserver echoclient echoserver-udp echoclient-udp \
	 hfetch sdr dnstest

jmtransfer: jmreceive jmsend

