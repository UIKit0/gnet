## Process this file with automake to produce Makefile.in

@SET_MAKE@

EXTRA_PROGRAMS = echoserver echoclient 	\
		 echoserver-udp echoclient-udp	\
		 echoserver-unix echoclient-unix \
		 hfetch sdr ljoust
EXTRA_DIST     = ljoust.c makefile.mingw
CLEANFILES     = $(EXTRA_PROGRAMS) ljoust

INCLUDES += `gnet-config --cflags`
LDFLAGS  += `gnet-config --libs`


##########
# Main examples

echoserver_SOURCES = 	\
	echoserver.c

echoclient_SOURCES =	\
	echoclient.c

echoserver_udp_SOURCES = \
	echoserver-udp.c

echoclient_udp_SOURCES = \
	echoclient-udp.c

echoserver_unix_SOURCES = \
	echoserver-unix.c

echoclient_unix_SOURCES = \
	echoclient-unix.c

sdr_SOURCES = \
	sdr.c

hfetch_SOURCES = 	\
	hfetch.c

ljoust:
	$(CC) ljoust.c -o ljoust `gtk-config --cflags --libs` `gnet-config --cflags --libs`



########################################

examples: echoserver echoclient 	  \
	  echoserver-udp echoclient-udp   \
	  echoserver-unix echoclient-unix \
	  hfetch sdr			  \
