## Process this file with automake to produce Makefile.in

SUBDIRS = src examples tests doc 

@SET_MAKE@

EXTRA_DIST=TODO HACKING BUGS  README.macosx gnetconfig.h.in \
	README.win32 GNET.DSP config.h.win32 gnet.def gnet.m4 \
	gnet.spec gnet.spec.in gnet.pc.in \
	examples/makefile.mingw src/makefile.mingw \
	src/gnet-private.h src/socks-private.h \
	tests/makefile.mingw


bin_SCRIPTS=gnet-config
BUILT_SOURCES=gnet-config
gnet-config: gnet-config.in

m4datadir = $(datadir)/aclocal
m4data_DATA = gnet.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gnet.pc

$(pkgconfig_DATA): config.status

configincludedir = $(pkglibdir)/include
configinclude_DATA = gnetconfig.h

WEBPATH=/n/mesh/home/www/gnetlibrary.org/html

DOCS=README TODO NEWS ChangeLog COPYING AUTHORS HACKING

TARBALL=gnet-$(VERSION).tar.gz

DEBFILE=libgnet$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION)_$(VERSION)-*_i386.deb
DEBFILEDEV=libgnet$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION)-dev_$(VERSION)-*_i386.deb
DEBSRC=gnet_$(VERSION)-1.tar.gz

RPMROOT=/usr/src/redhat
RPMFILE=$(RPMROOT)/RPMS/i386/gnet-$(VERSION)-*.i386.rpm
RPMFILEDEV=$(RPMROOT)/RPMS/i386/gnet-devel-$(VERSION)-*.i386.rpm
RPMFILESRC=$(RPMROOT)/SRPMS/gnet-$(VERSION)-*.src.rpm


snapshot:
	$(MAKE) dist distdir=$(PACKAGE)`date +"%y%m%d"`

copyweb:
	-cp -f $(DOCS) $(WEBPATH)
	-(cd $(WEBPATH); chmod a+r $(DOCS))
	-rm -f $(WEBPATH)/docs/*
	-cp -f doc/html/book1.html $(WEBPATH)/docs/index.html
	-cp -f doc/html/* $(WEBPATH)/docs
	-chmod a+r $(WEBPATH)/docs/*
	-cp -f $(TARBALL) $(WEBPATH)/src
	-cp -f $(RPMFILE) $(RPMFILEDEV) $(RPMFILESRC) $(WEBPATH)/rpm
	-chmod a+r $(WEBPATH)/src/* $(WEBPATH)/deb/* $(WEBPATH)/rpm/*

# You don't need to be root, but make sure you
# do 'make RPMROOT=/home/someone/redhat rpm' if
# you do not use the default $(RPMROOT).
rpm:
	rpm -ta $(TARBALL)
