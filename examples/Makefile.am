## Process this file with automake to produce Makefile.in

@SET_MAKE@

EXTRA_PROGRAMS = echoserver echoclient 	\
		 echoserver-udp echoclient-udp	\
		 echoserver-unix echoclient-unix \
		 hfetch sdr
EXTRA_DIST     = makefile.mingw
CLEANFILES     = $(EXTRA_PROGRAMS)

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



########################################

examples: echoserver echoclient 	  \
	  echoserver-udp echoclient-udp   \
	  echoserver-unix echoclient-unix \
	  hfetch sdr			  \
