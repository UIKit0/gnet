# Process this file with autoconf to produce a configure script.
#
# $Id$

# autoconf requirement
AC_PREREQ([2.52])

###############################
# Init autoconf and automake
AC_INIT([gnet], [1.1.6], [gnet@gnetlibrary.org])
# CANONICAL_TARGET has to occur before INIT_AUTOMAKE, for reasons I don't
# fully understand
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
# Activate maintainer mode
AM_MAINTAINER_MODE


###############################
# Package info
AC_REVISION([$Revision$])
AC_CONFIG_SRCDIR([src/gnet.c])

# On making releases:
#   GNET_MICRO_VERSION += 1;
#   GNET_INTERFACE_AGE += 1;
#   GNET_BINARY_AGE += 1;
# if any functions have been added, set GNET_INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set GNET_BINARY_AGE _and_ GNET_INTERFACE_AGE to 0.
GNET_MAJOR_VERSION=1
GNET_MINOR_VERSION=1
GNET_MICRO_VERSION=6
GNET_INTERFACE_AGE=0
GNET_BINARY_AGE=6
GNET_VERSION=$GNET_MAJOR_VERSION.$GNET_MINOR_VERSION.$GNET_MICRO_VERSION

AC_SUBST(GNET_MAJOR_VERSION)
AC_SUBST(GNET_MINOR_VERSION)
AC_SUBST(GNET_MICRO_VERSION)
AC_SUBST(GNET_VERSION)
AC_SUBST(GNET_INTERFACE_AGE)
AC_SUBST(GNET_BINARY_AGE)

# libtool versioning
LT_RELEASE=$GNET_MAJOR_VERSION.$GNET_MINOR_VERSION
LT_CURRENT=`expr $GNET_MICRO_VERSION - $GNET_INTERFACE_AGE`
LT_REVISION=$GNET_INTERFACE_AGE
LT_AGE=`expr $GNET_BINARY_AGE - $GNET_INTERFACE_AGE`
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
# Define version stuff
AC_DEFINE_UNQUOTED(GNET_MAJOR_VERSION, $GNET_MAJOR_VERSION, [GNet major version])
AC_DEFINE_UNQUOTED(GNET_MINOR_VERSION, $GNET_MINOR_VERSION, [GNet minor version] )
AC_DEFINE_UNQUOTED(GNET_MICRO_VERSION, $GNET_MICRO_VERSION, [GNet micro version])
AC_DEFINE_UNQUOTED(GNET_INTERFACE_AGE, $GNET_INTERFACE_AGE, [GNet interface age])
AC_DEFINE_UNQUOTED(GNET_BINARY_AGE, $GNET_BINARY_AGE, [GNet binary age])


####################
# Glib2

AC_ARG_ENABLE(glib2,
	      AC_HELP_STRING([--enable-glib2],
			     [prefer glib 2.0 over glib 1.2]))


####################
# Debug

AC_ARG_ENABLE(debug, [  --enable-debug=[no/minimum/yes] turn on debugging (default=minimum],,enable_debug=minimum)

AC_MSG_CHECKING([debug level])
if test "x$enable_debug" = "xyes"; then
  if test x$cflags_set != xset ; then
      case " $CFLAGS " in
      *[[\ \	]]-g[[\ \	]]*) ;;
      *) CFLAGS="$CFLAGS -g" ;;
      esac
  fi
	
  GNET_DEBUG_FLAGS="-DG_ENABLE_DEBUG"
else
  if test "x$enable_debug" = "xno"; then
    GNET_DEBUG_FLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS"
  fi
fi
AC_SUBST(GNET_DEBUG_FLAGS)
AC_MSG_RESULT($enable_debug)


###############################
# Check for programs
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_CHECK_PROG(PKGCONFIG, [pkg-config], [yes])
AM_PROG_LIBTOOL

###############################
# Check for libraries
AC_CHECK_LIB([socket], [main])
AC_CHECK_LIB([nsl], [main])
AC_CHECK_LIB([resolv], [main])

# Wacky pthread madness
pthread_libs=""
AC_CHECK_LIB([pthread],
	     [pthread_create],
	     [pthread_libs="-lpthread"])
AC_CHECK_LIB([c_r],
	     [pthread_create],
	     [pthread_libs="-lc_r"])
if test "$pthread_libs"; then
	AC_DEFINE(HAVE_LIBPTHREAD, 1,
		  [Define if pthreads library is available])
	LIBS="$pthread_libs $LIBS"
fi
AC_SUBST(pthread_libs)

if test "x${enable_glib2}" != "xyes"; then
	AM_PATH_GLIB([1.2.10],
		     ,
		     [enable_glib2=yes],
		     [gthread])
fi

if test "x${PKGCONFIG}" = "xyes" -a "x${enable_glib2}" = "xyes"; then
	# Check for 2.0, then 1.2
	PKG_CHECK_MODULES(GLIB,
			  [glib-2.0],
			  ,
			  [PKG_CHECK_MODULES(GLIB, glib)])
	PKG_CHECK_MODULES(GTHREAD,
			  [gthread-2.0],
			  ,
			  [PKG_CHECK_MODULES(GTHREAD, gthread)])
fi


# If pkg-config wasn't found, or didn't work
if test "x${GLIB_CFLAGS}" = "x"; then
   AC_MSG_ERROR([glib could not be found])
fi


# These SUBSTs are primarily for gnet-config and gnet.pc
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

###############################
# Check for headers
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/sockio.h sys/param.h])

###############################
# Check for types

###############################
# Check for structures
AC_CHECK_MEMBER([struct sockaddr.sa_len],
		AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, 
		  [Define if struct sockaddr has sa_len field]),,
		[#include <sys/types.h>
		 #include <sys/socket.h>])

###############################
# Compiler characteristics
AC_C_CONST

# If the compiler is gcc, turn on as many warnings as possible
if test "x$GCC" = "xyes"; then
	case " $CFLAGS " in
		*[\ \	]-Wall[\ \	]*)
			;;
		*)
			CFLAGS="$CFLAGS -Wall -Wstrict-prototypes \
			-Wmissing-prototypes -Wmissing-declarations"
			;;
	esac
	if test "x$enable_ansi" = "xyes"; then
		case " $CFLAGS " in
			*[\ \	]-ansi[\ \	]*)
				;;
			*)
				CFLAGS="$CFLAGS -ansi"
				;;
		esac
		case " $CFLAGS " in
			*[\ \	]-pedantic[\ \	]*)
				;;
			*)
				CFLAGS="$CFLAGS -pedantic"
				;;
		esac
	fi
fi
# Use reentract functions, and compile expiramental stuff
CFLAGS="$CFLAGS -D_REENTRANT -DGNET_EXPERIMENTAL"

###############################
# Check for library functions

# Look for a gethostbyname_r we can use (we assume that if the use has
# gethostbyname_r, they also have gethostbyaddr_r)

# First, look for the glibc varient
AC_MSG_CHECKING([for glibc gethostbyname_r])
AC_TRY_LINK([#include <netdb.h>],
	    [struct hostent result_buf;
	     char buf[1024];
	     struct hostent *result;
	     int h_erropp;

	     gethostbyname_r("localhost",
			     &result_buf,
			     buf,
			     sizeof(buf),
			     &result,
			     &h_erropp);],
	     [AC_MSG_RESULT([yes])
	      AC_DEFINE(HAVE_GETHOSTBYNAME_R_GLIBC, 1, 
	         [Define if glibc variant of gethostbyname is available])
	      HAVE_GETHOSTBYNAME_R=yes],
	     [AC_MSG_RESULT([no])])

# Whups, maybe we're on Solaris or Irix
AC_MSG_CHECKING([for Solaris/Irix gethostbyname_r])
AC_TRY_LINK([#include <netdb.h>],
	    [struct hostent result;
	     char buf[1024];
	     int h_errnop;

	     gethostbyname_r("localhost",
			     &result,
			     buf,
			     sizeof(buf),
			     &h_errnop);],
	    [AC_MSG_RESULT([yes])
	     AC_DEFINE(HAVE_GETHOSTBYNAME_R_SOLARIS, 1,
                [Define if Solaris variant of gethostbyname is available])
	     HAVE_GETHOSTBYNAME_R=yes],
	    [AC_MSG_RESULT([no])])

# Or not.  Perhaps we're on HP-UX?
AC_MSG_CHECKING([for HP-UX gethostbyname_r])
AC_TRY_LINK([#include <netdb.h>],
	    [struct hostent result;
	     char buf[1024];

	     gethostbyname_r("localhost",
			     &result,
			     buf);],
	    [AC_MSG_RESULT([yes])
	     AC_DEFINE(HAVE_GETHOSTBYNAME_R_HPUX, 1,
                [Define if HPUX variant of gethostbyname is available])
	     HAVE_GETHOSTBYNAME_R=yes],
	    [AC_MSG_RESULT([no])])

# We're stuck without gethostbyname_r.  Use Glib threads.
if test -z "$HAVE_GETHOSTBYNAME_R" -a "$GTHREAD_CFLAGS"; then
	AC_DEFINE(HAVE_GETHOSTBYNAME_R_GLIB_MUTEX, 1,
                [Define if gethostbyname requires a mutex for thread safety])
	HAVE_GETHOSTBYNAME_R=yes
fi

# Duh-oh.  We don't have _any_ thread safety, so warn the builder
if test -z "$HAVE_GETHOSTBYNAME_R"; then
	AC_MSG_WARN([You have neither gethostbyname_r() or Glib
threads.  This means that calls to gethostbyname() (called by the Gnet
address functions) will not be thread safe.  This build of Gnet could
malfunction if used in programs that use threads.])  
fi
		
###############################
# Outputs
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([
	gnet-config
	Makefile
	src/Makefile
	examples/Makefile
	tests/Makefile
	doc/Makefile
	doc/gnet-config.1
	gnetconfig.h
	gnet.spec
	gnet.pc],
	[chmod -f +x gnet-config])
AC_OUTPUT
