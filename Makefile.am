## Process this file with automake to produce Makefile.in

SUBDIRS = src examples doc

@SET_MAKE@

EXTRA_DIST=TODO HACKING	gnet.m4

bin_SCRIPTS=gnet-config
BUILT_SOURCES=gnet-config
gnet-config: gnet-config.in

m4datadir = $(datadir)/aclocal
m4data_DATA = gnet.m4

WEBPATH=$(HOME)/public_html/misc/gnet

TARBALL=gnet-$(VERSION).tar.gz
DEBFILE=libgnet$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION)_$(VERSION)-*_i386.deb
DEBFILEDEV=libgnet$(GNET_MAJOR_VERSION).$(GNET_MINOR_VERSION)-dev_$(VERSION)-*_i386.deb

snapshot:
	$(MAKE) dist distdir=$(PACKAGE)`date +"%y%m%d"`

copyweb:
	-cp -f README TODO NEWS ChangeLog COPYING AUTHORS HACKING $(WEBPATH)
	-cp -f $(TARBALL) $(WEBPATH)
	-(cd $(WEBPATH); chmod a+r README TODO NEWS ChangeLog \
		COPYING AUTHORS HACKING $(TARBALL))
	-cp -f debian-temp/$(DEBFILE) debian-temp/$(DEBFILEDEV) $(WEBPATH)
	-chmod a+r $(WEBPATH)/*.deb

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

clean-local:
	rm -rf debian-temp

