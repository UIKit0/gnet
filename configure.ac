# Process this file with autoconf to produce a configure script.
#
# $Id$

# autoconf requirement
AC_PREREQ([2.53])

# Init autoconf and automake
AC_INIT([gnet], [2.0.7], [gnet@gnetlibrary.org])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_MAINTAINER_MODE
AC_REVISION([$Revision$])
AC_CONFIG_SRCDIR([src/gnet.c])

# On making releases:
#   GNET_MICRO_VERSION += 1;
#   GNET_INTERFACE_AGE += 1;
#   GNET_BINARY_AGE += 1;
# if any functions have been added, set GNET_INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
# set GNET_BINARY_AGE _and_ GNET_INTERFACE_AGE to 0.
GNET_MAJOR_VERSION=2
GNET_MINOR_VERSION=0
GNET_MICRO_VERSION=7
GNET_INTERFACE_AGE=1
GNET_BINARY_AGE=7
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
LT_CURRENT_MINUS_AGE=`expr $LT_CURRENT - $LT_AGE`
AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)
AC_SUBST(LT_CURRENT_MINUS_AGE)
# Define version stuff
AC_DEFINE_UNQUOTED(GNET_MAJOR_VERSION, $GNET_MAJOR_VERSION, [GNet major version])
AC_DEFINE_UNQUOTED(GNET_MINOR_VERSION, $GNET_MINOR_VERSION, [GNet minor version] )
AC_DEFINE_UNQUOTED(GNET_MICRO_VERSION, $GNET_MICRO_VERSION, [GNet micro version])
AC_DEFINE_UNQUOTED(GNET_INTERFACE_AGE, $GNET_INTERFACE_AGE, [GNet interface age])
AC_DEFINE_UNQUOTED(GNET_BINARY_AGE, $GNET_BINARY_AGE, [GNet binary age])

AC_MSG_CHECKING([for Win32])
case "$host" in
  *-*-mingw*)
    gnet_native_win32=yes
    # Why not use G_OS_WIN32?
    AC_DEFINE_UNQUOTED(GNET_WIN32, 1, [Define when compiling on Windows])
    AC_DEFINE(HAVE_IPV6, 1, [Define if IPv6 is potentially available])
    ;;
  *)
    gnet_native_win32=no
    ;;
esac
AC_MSG_RESULT([$gnet_native_win32])
AM_CONDITIONAL(OS_WIN32, [test "$gnet_native_win32" = "yes"])

####################
# Glib1

AC_ARG_ENABLE(glib12,
	      AC_HELP_STRING([--enable-glib1.2],
			     [prefer GLib 1.2 over GLib 2]),,enable_glib12=no)

####################
# Debug

AC_ARG_ENABLE(debug, [  --enable-debug=[no/minimum/yes]  turn on debugging (default=minimum],,enable_debug=minimum)

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


# Network tests
AC_MSG_CHECKING([network tests])
AC_ARG_ENABLE(network_tests, 
	      AC_HELP_STRING([--enable-network-tests],
			     [enable tests that use the network]),
	      ,
              [enable_network_tests=no])
if test "x${enable_network_tests}" = "xyes"; then
   NETWORK_TESTS="client_server_test.pl dns_test.pl"
   AC_SUBST(NETWORK_TESTS)
fi
AC_MSG_RESULT($enable_network_tests)


if test "$gnet_native_win32" = "yes"; then
  if test x$enable_static = xyes -o x$enable_static = x; then
    AC_MSG_WARN([Disabling static library build, must build as DLL on Windows.])
    enable_static=no
  fi
  if test x$enable_shared = xno; then
    AC_MSG_WARN([Enabling shared library build, must build as DLL on Windows.])
  fi
  enable_shared=yes
fi

###############################
# Check for programs
AC_PROG_CC
AM_PROG_CC_STDC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_CHECK_PROG(PKGCONFIG, [pkg-config], [yes])
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(void*)

####################
# Mac OS X

# MacOSX doesn't define socklen_t, so we must define it to be an
# 'int'.  On all other systems, it's size_t.
case $build in
  *-apple-darwin*)
    AC_DEFINE(GNET_APPLE_DARWIN, 1,
              [Define if building on Apple Darwin])
esac


###############################
# Check for libraries
AC_CHECK_LIB([socket], [main])
AC_CHECK_LIB([nsl], [main])
AC_CHECK_LIB([resolv], [main])

# Check for pthreads
AC_ARG_ENABLE(pthreads,
             AC_HELP_STRING([--disable-pthreads],
                            [ignore pthreads library if present]),
             , # we don't need to do anything here
             enable_pthreads=yes)

if test "x${enable_pthreads}" = "xyes"; then
  PTHREAD_LIBS=""

  # Check for pthreads with -pthread
  AC_MSG_CHECKING([for -pthread])
  gnet_save_LIBS=$LIBS
  LIBS="$LIBS -pthread"
  dash_pthread=""
  PTHREAD_PROGRAM="
#include <pthread.h>
int main(void)
{  
   pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
   return 0;
}
"
  AC_TRY_RUN($PTHREAD_PROGRAM,
	[dash_pthread="-pthread"
	 AC_MSG_RESULT(yes)],
	[dash_pthread=""
         # Otherwise, check for pthreads with -pthreads
	 AC_MSG_RESULT(no)
         AC_MSG_CHECKING([for -pthreads])
         LIBS="$gnet_save_LIBS -pthreads"
	 AC_TRY_RUN($PTHREAD_PROGRAM,
	   [dash_pthread="-pthreads"
	   AC_MSG_RESULT(yes)],
	   [dash_pthread=""
	   AC_MSG_RESULT(no)])
	])
  LIBS="$gnet_save_LIBS $dash_pthread"

  # Otherwise, check for -lpthread and -lc_r
  if test "x$dash_pthread" == "x"; then
    AC_CHECK_LIB([pthread],
            [pthread_create],
            [PTHREAD_LIBS="-lpthread"])
    AC_CHECK_LIB([c_r],
            [pthread_create],
            [PTHREAD_LIBS="-lc_r $PTHREAD_LIBS"])
  fi    
  # If either -pthread[s] or -lpthread works, we have pthreads
  if test "x$PTHREAD_LIBS" != "x" -o "x$dash_pthread" != "x"; then
          AC_DEFINE(HAVE_LIBPTHREAD, 1,
                  [Define if pthreads library is available])
          LIBS="$PTHREAD_LIBS $LIBS"
  fi
fi

m4_ifdef([AM_PATH_GLIB],[],[AC_DEFUN([AM_PATH_GLIB],[AC_MSG_ERROR([No GLib 1.2 on this system])])])

# Check for GLib 1.2 if enabled
if test "x${enable_glib12}" = "xyes"; then
  AM_PATH_GLIB([1.2.10],
		# GTHREAD_CFLAGS are included in GLIB_CFLAGS for 1.2
		GTHREAD_CFLAGS=" ",  
		AC_MSG_ERROR([GLib 1.2 could not be found]),
		[gthread])
   
fi


# Check for GLib 2.0
if test "x${PKGCONFIG}" = "xyes" -a "x${enable_glib12}" != "xyes"; then

  # Check for GLib 2.0 (or 2.2)
  PKG_CHECK_MODULES(GLIB, [glib-2.0], [:], [:])
  PKG_CHECK_MODULES(GTHREAD, [gthread-2.0], [:], [:])

  # Check for GLib 1.2 (using pkg-config)
  if test "x${GLIB_CFLAGS}" = "x"; then

    PKG_CHECK_MODULES(GLIB, [glib], [:], [:])
    PKG_CHECK_MODULES(GTHREAD, [gthread], [:], [:])
  fi
fi


# Check for GLib 1.2 if no GLib 2.X
if test "x${GLIB_CFLAGS}" = "x"; then
  AM_PATH_GLIB([1.2.10],
		# GTHREAD_CFLAGS are included in GLIB_CFLAGS for 1.2
		GTHREAD_CFLAGS=" ",  
		,
		[gthread])
fi


# Fail if we didn't find GLib
if test "x${GLIB_CFLAGS}" = "x"; then
   AC_MSG_ERROR([GLib could not be found])
fi


# These SUBSTs are primarily for gnet-config and gnet.pc
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)

# Check for IPv6 support on unix
# (will be done at runtime on win32)
if test "x$gnet_native_win32" != "xyes"
then
  AC_MSG_CHECKING([for IPv6 availability])
  AC_TRY_LINK([ #include <sys/types.h>
                #include <sys/socket.h>
                #include <netinet/in.h> ], 
              [ struct sockaddr_in6 s; 
                struct in6_addr t=in6addr_any; 
                int i=AF_INET6; 
                s; 
                t.s6_addr[0] = 0; ],
              [
                AC_DEFINE(HAVE_IPV6, 1, [Define if IPv6 is potentially available])
                AC_MSG_RESULT(yes)
              ],
              [
                AC_MSG_RESULT(no)
              ]
  )
fi



###############################
# Check for headers
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/sockio.h sys/param.h ifaddrs.h])


AC_MSG_CHECKING([for linux/netlink.h])
AC_TRY_CPP([#include <linux/netlink.h>],
	   [
	     AC_MSG_RESULT(yes)	     
             AC_DEFINE(HAVE_LINUX_NETLINK_H, 1,
	       [Define if linux/netlink.h header exists])
           ],[
             AC_MSG_RESULT(no)
           ])



###############################
# Check for types


###############################
# Check for structures
AC_CHECK_MEMBER([struct sockaddr.sa_len],
		AC_DEFINE(HAVE_SOCKADDR_SA_LEN, 1, 
		  [Define if struct sockaddr has sa_len field]),,
		[#include <sys/types.h>
		 #include <sys/socket.h>])

AC_MSG_CHECKING([for struct sockaddr_storage])
AC_TRY_COMPILE([ #include <sys/types.h>
#include <sys/socket.h>], [struct sockaddr_storage s; s],
  have_sockaddr_storage=yes, have_sockaddr_storage=no)
if test "$have_sockaddr_storage" = yes; then
  AC_DEFINE(HAVE_SOCKADDR_STORAGE,1,[Whether you have struct sockaddr_storage])
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi


###############################
# Compiler characteristics
AC_C_CONST

# Use reentract functions, and compile expiramental stuff
CFLAGS="$CFLAGS -D_REENTRANT -DGNET_EXPERIMENTAL"


###############################
# Check for library functions

# Most of this code checks for variants of gethostbyname().  For
# simplicity and curiosity, we check for all variants, though only one
# working variant is needed.

# Look for getifaddrs()
AC_CHECK_FUNC(getifaddrs, AC_DEFINE(HAVE_GETIFADDRS, 1, 
    [Define if getifaddrs() is available]))


# The user may be able to tell us if a function is thread-safe.  We
# know of no good way to test this programaticly.

# On some systems, getaddrinfo() is thread-safe
AC_ARG_ENABLE(threadsafe_getaddrinfo,
	      AC_HELP_STRING([--enable-threadsafe-getaddrinfo],
			     [assume getaddrinfo() is threadsafe]))

# On some systems, gethostbyname() is thread-safe
AC_ARG_ENABLE(threadsafe_gethostbyname,
	      AC_HELP_STRING([--enable-threadsafe-gethostbyname],
			     [assume gethostbyname() is threadsafe]))


# Look for getaddrinfo().  If we have that, we don't need
# gethostbyname().  We assume if the system has getaddrinfo(), they
# have getnameinfo().

AC_CHECK_FUNC(getaddrinfo, 
 [ # have getaddrinfo()
  HAVE_GETADDRINFO=yes
  AC_DEFINE(HAVE_GETADDRINFO, 1, 
    [Define if getaddrinfo() is available])
  # Check for threadsafe getaddrinfo()
  AC_MSG_CHECKING([for threadsafe getaddrinfo()])
  if test "x$enable_threadsafe_getaddrinfo" = "xyes" ; then
    HAVE_GETADDRINFO_THREADSAFE=yes
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_GETADDRINFO_THREADSAFE, 1, 
	[Define if getaddrinfo is threadsafe])
  else
    AC_MSG_RESULT(no)
    # If there is no threadsafe getaddrinfo, use a Glib mutex.
    if test "x$GTHREAD_CFLAGS" != "x"; then
      AC_DEFINE(HAVE_GETADDRINFO_GLIB_MUTEX, 1,
                [Define if getaddrinfo requires a mutex for thread-safety])
	HAVE_GETADDRINFO_GLIB_MUTEX=yes
    else
	AC_MSG_WARN([

You do not have a thread-safe getaddrinfo() or GLib threads.
Calls to getaddrinfo() (called by the GNet address functions) may
not be thread-safe.  This build of GNet could malfunction if used in
programs that use threads.  If your system has a thread-safe version
of getaddrinfo(), pass the option --enable-threadsafe-getaddrinfo
to configure.  The function's manpage may be able to tell you whether
it is thread-safe or not.

])
    fi
  fi
 ],
 [  # no getaddrinfo()
  AC_MSG_RESULT(no)
 ])



# Check if the user told use gethostbyname() is thread-safe.
AC_MSG_CHECKING([for threadsafe gethostbyname()])
if test "x$enable_threadsafe_gethostbyname" = "xyes" ; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_GETHOSTBYNAME_THREADSAFE, 1, 
	[Define if gethostbyname is threadsafe])
  HAVE_GETHOSTBYNAME_THREADSAFE=yes
else
  AC_MSG_RESULT(no)
fi

# Look for a gethostbyname_r() we can use.  We assume that if the
# system has gethostbyname_r(), they also have gethostbyaddr_r().

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
	         [Define if glibc variant of gethostbyname() is available])
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
                [Define if Solaris variant of gethostbyname() is available])
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
               [Define if HPUX variant of gethostbyname() is available])
	     HAVE_GETHOSTBYNAME_R=yes],
	    [AC_MSG_RESULT([no])])

# If there is no gethostbyname_r, use Glib threads.
if test -z "$HAVE_GETHOSTBYNAME_THREADSAFE" -a -z "$HAVE_GETHOSTBYNAME_R" -a "$GTHREAD_CFLAGS"; then
	AC_DEFINE(HAVE_GETHOSTBYNAME_R_GLIB_MUTEX, 1,
                [Define if gethostbyname requires a mutex for thread-safety])
	HAVE_GETHOSTBYNAME_R=yes
fi

# Duh-oh.  We don't have _any_ thread-safety, so warn the builder
if test -z "$HAVE_GETADDRINFO_THREADSAFE" -a -z "$HAVE_GETHOSTBYNAME_THREADSAFE" -a -z "$HAVE_GETHOSTBYNAME_R"; then
	AC_MSG_WARN([

You do not have a thread-safe gethostbyname_r() or GLib threads.
Calls to gethostbyname() (called by the GNet address functions) may
not be thread-safe.  This build of GNet could malfunction if used in
programs that use threads.  If your system has a thread-safe version
of gethostbyname(), pass the option --enable-threadsafe-gethostbyname
to configure.  The function's manpage may be able to tell you whether
it is thread-safe or not.

])
fi


# If the compiler is gcc, turn on as many warnings as possible
if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -Wstrict-prototypes \
    -Wmissing-prototypes -Wmissing-declarations"
  if test "x$enable_debug" = "xyes"; then
    CFLAGS="$CFLAGS -Werror"
  fi

  if test "x$enable_ansi" = "xyes"; then
    CFLAGS="$CFLAGS -ansi"
    CFLAGS="$CFLAGS -pedantic"
  fi
fi

	
###############################
# GTK-DOC
###############################

GTK_DOC_CHECK(1.2)

###############################
# Outputs
###############################

AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([
	Makefile
	src/Makefile
	examples/Makefile
	tests/Makefile
	doc/Makefile
	doc/version.xml
	gnetconfig.h
	gnet.spec
	gnet-2.0.pc])
AC_OUTPUT
