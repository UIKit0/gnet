## Process this file with automake to produce Makefile.in

SUBDIRS = src examples tests doc 

@SET_MAKE@

EXTRA_DIST=TODO HACKING BUGS  README.macosx gnetconfig.h.in \
	README.win32 GNET.DSP config.h.win32 gnet.def gnet.m4 \
	gnet.spec gnet.spec.in gnet.pc.in \
	debian/README.Debian debian/changelog debian/control.in \
	debian/copyright debian/dirs debian/docs debian/libgnet1.1* \
	debian/rules.in

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

WEBPATH=/n/misc/home/www/gnetlibrary.org/html
DEBPATH=$(HOME)/public_html/misc/debian

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

# You don't need to be root, but make sure you
# do 'make RPMROOT=/home/someone/redhat rpm' if
# you do not use the default $(RPMROOT).
rpm:
	rpm -ta $(TARBALL)

clean-local:
	rm -rf debian-temp
