#!/bin/sh
# Gnome autogen.sh

AUTOCONF_BIN=autoconf
AUTOHEADER_BIN=autoheader
AUTOMAKE_BIN=automake
ACLOCAL_BIN=aclocal

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


AUTOMAKE_VERSION_REAL=`automake --version | head -n 1 | awk '{print $4}'`
AUTOMAKE_VERSION=`echo $AUTOMAKE_VERSION_REAL | sed -e 's/\.\([0-9]\).*/\1/'`

echo "***** Checking automake version: $AUTOMAKE_VERSION_REAL"

if [ "x$AUTOMAKE_VERSION" == "x14" ] || [ "x$AUTOMAKE_VERSION" == "x15" ]
then
  echo "***** Checking for newer automake versions...";

  FOUND_AM=0

  for ver in 2.0 1.9 1.8 1.7 1.6
  do
    if automake-$ver --version </dev/null &>/dev/null
    then
      AUTOMAKE_BIN=automake-$ver;
      ACLOCAL_BIN=aclocal-$ver;
      echo "***** Found $AUTOMAKE_BIN. Using that instead.";
      FOUND_AM=1
      break
    fi
  done

  if [ "x$FOUND_AM" = "x0" ]; then
    echo "***** Your automake version (1.4 or 1.5) is too old.";
    echo "***** Please install a newer version (1.9, 1.8, 1.7 or 1.6)";
  fi

fi

#-----------------------------------------------------------------------------#
# ... and the same for autoconf ...
#-----------------------------------------------------------------------------#

# This only seems not to work with autoconf2.13, but does work with 2.5x
AUTOCONF_VERSION_REAL=`$AUTOCONF_BIN --version | head -n 1 | awk '{print $4}'`

if [ -z $AUTOCONF_VERSION_REAL ]; then
  AUTOCONF_VERSION_REAL=`$AUTOCONF_BIN --version | head -n 1 | awk '{print $3}'`
fi

echo "***** Checking autoconf version: $AUTOCONF_VERSION_REAL"

# Check whether the autoconf version detected is too old.
FOUND_OLD_AC=0
for ver in 2.10 2.11 2.12 2.13
do
  if [ "x$AUTOCONF_VERSION_REAL" = "x$ver" ]
  then
    FOUND_OLD_AC=1
    break
  fi
done

if [ "x$FOUND_OLD_AC" = "x1" ];
then
  echo "***** Checking for newer autoconf versions..."

  FOUND_NEW_AC=0

  for ver in 2.53 2.54 2.55 2.56 2.57 2.58 2.60
  do
    if autoconf$ver --version </dev/null &>/dev/null
    then
      AUTOCONF_BIN=autoconf$ver;
      AUTOHEADER_BIN=autoheader$ver;
      echo "***** Found $AUTOCONF_BIN. Using that instead.";
      FOUND_NEW_AC=1
      break
    fi
  done

  if [ "x$FOUND_NEW_AC" = "x0" ]; then
    echo "***** Your autoconf version (<=2.52) is too old.";
    echo "***** Please install a newer version (>=2.53)";
  fi
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
      $ACLOCAL_BIN $aclocalinclude || { echo "$ACLOCAL_BIN failed!"; exit 1; }

      if grep "^AM_CONFIG_HEADER" configure.ac >/dev/null; then
        echo "Running autoheader..."
        $AUTOHEADER_BIN || { echo "$AUTOHEADER_BIN failed!"; exit 1; }
      fi
      echo "Running automake --gnu $am_opt ..."
      $AUTOMAKE_BIN --add-missing --gnu $am_opt || { echo "$AUTOMAKE_BIN failed!"; exit 1; }
      echo "Running autoconf ..."
      $AUTOCONF_BIN || { echo "$AUTOCONF_BIN failed!"; exit 1; }
    )
  fi
done

conf_flags="--enable-maintainer-mode --enable-compile-warnings --enable-debug=yes --enable-gtk-doc=yes --enable-network-tests=yes"

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile $PKG_NAME || exit 1
else
  echo Skipping configure process.
fi
