## Process this file with automake to produce Makefile.in

SUBDIRS = src examples tests doc 

@SET_MAKE@

EXTRA_DIST=TODO HACKING BUGS gnet.m4 gnetconfig.h.in

bin_SCRIPTS=gnet-config
BUILT_SOURCES=gnet-config
gnet-config: gnet-config.in

m4datadir = $(datadir)/aclocal
m4data_DATA = gnet.m4

configincludedir = $(pkglibdir)/include
configinclude_DATA = gnetconfig.h

WEBPATH=/n/misc/home/www/gnetlibrary.org/html
DEBPATH=$(HOME)/public_html/misc/debian

DOCS=README TODO NEWS ChangeLog COPYING AUTHORS HACKING

TARBALL=gnet-$(VERSION).tar.gz

DEBFILE=libgnet$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION)_$(VERSION)-*_i386.deb
DEBFILEDEV=libgnet$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION)-dev_$(VERSION)-*_i386.deb
DEBSRC=gnet_$(VERSION)-1.tar.gz

RPMFILE=/usr/src/redhat/RPMS/i386/gnet-$(VERSION)-*.i386.rpm
RPMFILEDEV=/usr/src/redhat/RPMS/i386/gnet-devel-$(VERSION)-*.i386.rpm
RPMFILESRC=/usr/src/redhat/SRPMS/gnet-$(VERSION)-*.src.rpm


snapshot:
	$(MAKE) dist distdir=$(PACKAGE)`date +"%y%m%d"`

copyweb:
	-cp -f $(DOCS) $(WEBPATH)
	cp -f doc/html/* $(WEBPATH)/docs
	cp -f doc/html/book1.html doc/html/index.html
	-(cd $(WEBPATH); chmod a+r $(DOCS))
	-cp -f $(TARBALL) $(WEBPATH)/src
	-cp -f debian-temp/$(DEBFILE) debian-temp/$(DEBFILEDEV) $(WEBPATH)/deb
	-cp -f debian-temp/$(DEBSRC) $(WEBPATH)/deb
	-cp -f $(RPMFILE) $(RPMFILEDEV) $(RPMFILESRC) $(WEBPATH)/rpm
	-chmod a+r $(WEBPATH)/src/* $(WEBPATH)/deb/* $(WEBPATH)/rpm/*
	-rm -f $(DEBPATH)/dists/unstable/main/binary-i386/libs/libgnet*
	-cp -f debian-temp/$(DEBFILE) debian-temp/$(DEBFILEDEV) $(DEBPATH)/dists/unstable/main/binary-i386/libs
	-(cd $(DEBPATH); ./setup.sh)

deb:
	rm -rf debian-temp
	-mkdir debian-temp
	cp $(TARBALL) debian-temp
	(cd debian-temp; tar xzf $(TARBALL))
	cp -R debian debian-temp/gnet-$(VERSION)
	(cd debian-temp/gnet-$(VERSION); dpkg-buildpackage -rfakeroot)
	echo Running lintian on $(DEBFILE)...
	lintian debian-temp/$(DEBFILE)
	echo Running lintian on $(DEBFILEDEV)...
	lintian debian-temp/$(DEBFILEDEV)

rpm:
	echo Make sure you are root!
	cp $(TARBALL) /usr/src/redhat/SOURCES
	rpm -ba gnet.spec

clean-local:
	rm -rf debian-temp

