#!/bin/sh
# Gnome autogen.sh


srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(test -f $srcdir/configure.ac) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level directory"
    exit 1
}


DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed to compile GNet."
  echo "Get it at ftp://ftp.gnu.org/pub/gnu/autoconf"
  DIE=1
}

(grep "^AM_PROG_LIBTOOL" $srcdir/configure.ac >/dev/null) && {
  (libtool --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed to compile GNet."
    echo "Get it at ftp://ftp.gnu.org/pub/gnu/libtool"
    DIE=1
  }
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed to compile GNet."
  echo "Get it at ftp://ftp.gnu.org/pub/gnu/automake"
  DIE=1
  NO_AUTOMAKE=yes
}


# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'."
  echo "aclocal is included with automake."
  echo "Get automake at ftp://ftp.gnu.org/pub/gnu/automake"
  DIE=1
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0\'" command line."
  echo
fi


case $CC in
xlc )
  am_opt=--include-deps;;
esac

for coin in `find $srcdir -name configure.ac -print`
do 
  dr=`dirname $coin`
  if test -f $dr/NO-AUTO-GEN; then
    echo skipping $dr -- flagged as no auto-gen
  else
    echo processing $dr
    macrodirs=`sed -n -e 's,AM_ACLOCAL_INCLUDE(\(.*\)),\1,gp' < $coin`
    ( cd $dr
      aclocalinclude="$ACLOCAL_FLAGS"
      if test -d ./macros; then
	aclocalinclude="$aclocalinclude -I ./macros"
      fi
      for k in $macrodirs; do
  	if test -d $k; then
          aclocalinclude="$aclocalinclude -I $k"
  	##else 
	##  echo "**Warning**: No such directory \`$k'.  Ignored."
        fi
      done
      if grep "^AM_PROG_LIBTOOL" configure.ac >/dev/null; then
	echo "Making compatibility symlink for libtool"
	ln -s configure.ac configure.in
	echo "Running libtoolize --force --copy..."
	libtoolize --force --copy
	echo "Removing compatibility symlink for libtool"
	rm -f configure.in
      fi
      # Get new versions of config.sub & config.guess if present,
      # otherwise we'll stick with the libtool varients
      # Note that these are provided by the Debian autotools-dev package
      test -r /usr/share/misc/config.sub &&
	ln -sf /usr/share/misc/config.sub config.sub
      test -r /usr/share/misc/config.guess &&
	ln -sf /usr/share/misc/config.guess config.guess
	
      echo "Running aclocal $aclocalinclude ..."
      aclocal $aclocalinclude
      if grep "^AM_CONFIG_HEADER" configure.ac >/dev/null; then
	echo "Running autoheader..."
	autoheader
      fi
      echo "Running automake --gnu $am_opt ..."
      automake --add-missing --gnu $am_opt
      echo "Running autoconf ..."
      autoconf
    )
  fi
done

conf_flags="--enable-maintainer-mode --enable-compile-warnings --enable-debug=yes"

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile $PKG_NAME || exit 1
else
  echo Skipping configure process.
fi
