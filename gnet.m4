# Configure paths for Gnet
# A hacked up version of Owen Taylor's gtk.m4 (Copyright 1997)

# Owen Taylor     97-11-3

dnl AM_PATH_GNET([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GNET, and define GNET_CFLAGS and GNET_LIBS
dnl
AC_DEFUN(AM_PATH_GNET,
[dnl 
dnl Get the cflags and libraries from the gnet-config script
dnl
AC_ARG_WITH(gnet-prefix,[  --with-gnet-prefix=PFX   Prefix where Gnet is installed (optional)],
            gnet_config_prefix="$withval", gnet_config_prefix="")
AC_ARG_WITH(gnet-exec-prefix,[  --with-gnet-exec-prefix=PFX Exec prefix where GNet is installed (optional)],
            gnet_config_exec_prefix="$withval", gnet_config_exec_prefix="")
AC_ARG_ENABLE(gnettest, [  --disable-gnettest       Do not try to compile and run a test GNet program],
		    , enable_gnettest=yes)

  if test x$gnet_config_exec_prefix != x ; then
     gnet_config_args="$gnet_config_args --exec-prefix=$gnet_config_exec_prefix"
     if test x${GNET_CONFIG+set} != xset ; then
        GNET_CONFIG=$gnet_config_exec_prefix/bin/gnet-config
     fi
  fi
  if test x$gnet_config_prefix != x ; then
     gnet_config_args="$gnet_config_args --prefix=$gnet_config_prefix"
     if test x${GNET_CONFIG+set} != xset ; then
        GNET_CONFIG=$gnet_config_prefix/bin/gnet-config
     fi
  fi

  AC_PATH_PROG(GNET_CONFIG, gnet-config, no)
  min_gnet_version=ifelse([$1], ,0.1.0,$1)
  AC_MSG_CHECKING(for GNET - version >= $min_gnet_version)
  no_gnet=""
  if test "$GNET_CONFIG" = "no" ; then
    no_gnet=yes
  else
    GNET_CFLAGS=`$GNET_CONFIG $gnet_config_args --cflags`
    GNET_LIBS=`$GNET_CONFIG $gnet_config_args --libs`
    gnet_config_major_version=`$GNET_CONFIG $gnet_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gnet_config_minor_version=`$GNET_CONFIG $gnet_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gnet_config_micro_version=`$GNET_CONFIG $gnet_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gnettest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GNET_CFLAGS"
      LIBS="$GNET_LIBS $LIBS"
dnl
dnl Now check if the installed GNet is sufficiently new. (Also sanity
dnl checks the results of gnet-config to some extent
dnl
      rm -f conf.gnettest
      AC_TRY_RUN([
#include <gnet/gnet.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gnettest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_gnet_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gnet_version");
     exit(1);
   }

  if ((gnet_major_version != $gnet_config_major_version) ||
      (gnet_minor_version != $gnet_config_minor_version) ||
      (gnet_micro_version != $gnet_config_micro_version))
    {
      printf("\n*** 'gnet-config --version' returned %d.%d.%d, but GNet (%d.%d.%d)\n", 
             $gnet_config_major_version, $gnet_config_minor_version, $gnet_config_micro_version,
             gnet_major_version, gnet_minor_version, gnet_micro_version);
      printf ("*** was found! If gnet-config was correct, then it is best\n");
      printf ("*** to remove the old version of GNet. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If gnet-config was wrong, set the environment variable GNET_CONFIG\n");
      printf("*** to point to the correct copy of gnet-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
#if defined (GNET_MAJOR_VERSION) && defined (GNET_MINOR_VERSION) && defined (GNET_MICRO_VERSION)
  else if ((gnet_major_version != GNET_MAJOR_VERSION) ||
	   (gnet_minor_version != GNET_MINOR_VERSION) ||
           (gnet_micro_version != GNET_MICRO_VERSION))
    {
      printf("*** GNet header files (version %d.%d.%d) do not match\n",
	     GNET_MAJOR_VERSION, GNET_MINOR_VERSION, GNET_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     gnet_major_version, gnet_minor_version, gnet_micro_version);
    }
#endif /* defined (GNET_MAJOR_VERSION) ... */
  else
    {
      if ((gnet_major_version > major) ||
        ((gnet_major_version == major) && (gnet_minor_version > minor)) ||
        ((gnet_major_version == major) && (gnet_minor_version == minor) && (gnet_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GNet (%d.%d.%d) was found.\n",
               gnet_major_version, gnet_minor_version, gnet_micro_version);
        printf("*** You need a version of GNet newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GNet is always available from http://www.gnetlibrary.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the gnet-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GNet, but you can also set the GNET_CONFIG environment to point to the\n");
        printf("*** correct copy of gnet-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_gnet=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gnet" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$GNET_CONFIG" = "no" ; then
       echo "*** The gnet-config script installed by GNet could not be found"
       echo "*** If GNet was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GNET_CONFIG environment variable to the"
       echo "*** full path to gnet-config."
     else
       if test -f conf.gnettest ; then
        :
       else
          echo "*** Could not run GNet test program, checking why..."
          CFLAGS="$CFLAGS $GNET_CFLAGS"
          LIBS="$LIBS $GNET_LIBS"
          AC_TRY_LINK([
#include <gnet/gnet.h>
#include <stdio.h>
],      [ return ((gnet_major_version) || (gnet_minor_version) || (gnet_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GNet or finding the wrong"
          echo "*** version of GNet. If it is not finding GNet, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the GNet package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps gnet gnet-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GNet was incorrectly installed"
          echo "*** or that you have moved GNet since it was installed. In the latter case, you"
          echo "*** may want to edit the gnet-config script: $GNET_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GNET_CFLAGS=""
     GNET_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GNET_CFLAGS)
  AC_SUBST(GNET_LIBS)
  rm -f conf.gnettest
])
