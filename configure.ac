# $Id: configure.ac,v 1.15 2004/10/03 20:44:55 cabbey Exp $
dnl	This file is an input file used by the GNU "autoconf" program to
dnl	generate the file "configure", which is run to configure the
dnl	Makefile in this directory.

dnl Set up the name and version
AC_INIT([jikes],[1.22])
AM_INIT_AUTOMAKE([foreign check-news dist-bzip2 1.8.3])
AC_PREREQ(2.59)

AC_DEFINE(JIKES_VERSION_STRING,
          "Version 1.22 - 3 October 2004",
    [A string to indicate the release of jikes: Version 1.19 - 1 Feb 2004])

dnl AC_CACHE_LOAD

AC_CONFIG_HEADERS([src/config.h])
AH_TOP([/* This software is subject to the terms of the IBM Public
License available at the following URL:
http://opensource.com/licenses/IPL-1.0
Copyright (C) 1996, 2004 IBM Corporation and others.  All Rights Reserved.
You must accept the terms of that agreement to use this software. */])

dnl	Do not autoupdate if configure.in and friends are changed
dnl	unless the --enable-maintainer-mode flag is used.
AM_MAINTAINER_MODE()

dnl	Set up host checks using config.sub and config.guess.
AC_CANONICAL_HOST()

dnl	VC++ requires special flags:
dnl	/TP  : Treat input files as c++
dnl	/GX  : Enable C++ exceptions
dnl	/GR  : Enable rtti
dnl	/MD  : Link to msvcrt.lib
dnl	/MDd : Link to msvcrtd.lib
dnl	/Zi  : Add debug info to .obj files
case "${host}" in
    *windows32*)
        jikes_vcpp=yes
        ;;
esac

dnl Maintainer mode depends on touch
AC_PATH_PROG([TOUCH], [touch])

dnl     make it easy to turn on debug, like any other project
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],
                              [Turn on internal compiler debugging.])],
          [jikes_debug=$enableval], [jikes_debug=no])

AS_IF([test "$jikes_debug" = yes],
  [AC_DEFINE([JIKES_DEBUG], [1], [Enable internal compiler debugging])

  dnl If the user did not set CXXFLAGS, set debug flags to
  dnl keep the AC_PROG_CXX macro from defaulting to -g -O2.
  if test "${CXXFLAGS+set}" != "set" ; then
    if test "$jikes_vcpp" = "yes" ; then
      CXXFLAGS="-Zi -MDd -GX -GR"
    else
      CXXFLAGS="-g"
    fi
  fi],
  [dnl If the user did not set CXXFLAGS and we are using VC++, set
  dnl default flags to keep AC_PROG_CXX from defaulting to -g -O2.
  if test "${CXXFLAGS+set}" != "set" ; then
    if test "$jikes_vcpp" = "yes" ; then
      CXXFLAGS="-MD -GX -GR"
    fi
  fi])

dnl	Tell the system that we are going to use the C++ language
AC_LANG([C++])

dnl	Have the system search for and test out the C++ compiler
dnl	We expect AC_PROG_CXX to automatically find a cross compiler
AC_PROG_CXX()

dnl	See if this host uses DOS style path names
ACX_CHECK_PATHNAME_STYLE_DOS()

dnl FIXME: This is a quick hack to define UNIX_FILE_SYSTEM
dnl or WIN32_FILE_SYSTEM, we need to go back and fix this
dnl to do a feature test in each place this symbol is used
AS_IF([test "$acx_cv_pathname_style_dos" = yes],
      [AC_DEFINE([WIN32_FILE_SYSTEM], [],
                 [quick hack to use Win32 file system, fixme])],
      [AC_DEFINE([UNIX_FILE_SYSTEM], [],
                 [quick hack to use UNIX file system, fixme])])

dnl	Don't define classes in a Jikes namespace by default
dnl	since most compilers do not support C++ namespaces properly.

AC_ARG_ENABLE([jikes-namespace],
              [AS_HELP_STRING([--enable-jikes-namespace],
                              [Define Jikes classes in a C++ namespace])],
    [ok=$enableval], [ok=no])

AS_IF([test "$ok" = yes],
      [AC_DEFINE([HAVE_JIKES_NAMESPACE], [1],
                 [Define Jikes classes in a C++ namespace])])


dnl	Check for standard c and c++ headers and libraries
AC_HEADER_STDC()

AC_CHECK_HEADERS(stdio.h assert.h errno.h limits.h ctype.h time.h float.h)

dnl	Check for some headers which are not present on all supported platforms
AC_CHECK_HEADERS(windows.h direct.h dirent.h sys/cygwin.h locale.h)

dnl	Check for math.h and see if we need to link to the math library
AC_CHECK_MATHLIB()

AC_CXX_HAVE_STD()

dnl	see if "bool" is a builtin type
AC_CXX_BOOL()

dnl	Check for <new> style include for the set_new_handler function
AC_CXX_CHECK_SET_NEW_HANDLER()

dnl	Check for wide string functions
AC_CHECK_WCS_FUNCS()

dnl     Check for busted ostream insertion problems
AC_CXX_CHECK_OSTREAM_CONST_UNSIGNED_CHAR_PTR()

dnl     The check for iconv might abort, so save cache
AC_CACHE_SAVE

dnl	Check for iconv functions and library
AC_CHECK_ICONV()

dnl	Check if "IBM Classes for UNICODE library" is present
AC_CHECK_LIB(icu-uc, u_getVersion)

AS_IF([test -n "$ac_cv_jikes_iconv_encoding" || \
       test "$ac_cv_lib_icu_uc_u_getVersion" = "yes"],
      [AC_DEFINE([HAVE_ENCODING], [1],
       [Defined if Jikes supports -encoding for input files.])])

dnl	Check for cygwin path conversion functions
AC_CHECK_FUNCS(cygwin_win32_to_posix_path_list)

dnl	Check for the strip utility, we use it to remove debugging info
AC_CHECK_TOOL(STRIP, strip, :)

dnl	Check to see if "static const FOO = 1" is allowed in a class
AC_CXX_MEMBER_CONSTANTS()

dnl	Check to see if the explicit C++ keyword is supported
AC_CXX_EXPLICIT()

dnl	Check to see if RTTI is implemented, it is needed for better dynamic
dnl     casting.
AC_CXX_RTTI()

dnl	Check for newer C++ style cast support
AC_CXX_CONST_CAST()
AC_CXX_DYNAMIC_CAST()
AC_CXX_REINTERPRET_CAST()
AC_CXX_STATIC_CAST()

dnl Check to see if build will use emulated floating point

AC_ARG_ENABLE([fp-emulation],
              [AS_HELP_STRING([--disable-fp-emulation],
                 [Use native floating point operations instead of emulation])],
	[ok=$enableval], [ok=yes])

AC_MSG_CHECKING([for floating point support])

AS_IF([test "$ok" = yes],
 [# Do nothing, floating point emulation used when HAVE_IEEE754 is not defined
  AC_MSG_RESULT([emulated])],
 [AC_DEFINE([HAVE_IEEE754], [1],
    [Defined when Jikes is built with native floating support])
  AC_MSG_RESULT([native])])

dnl Ensure that we have exact type sizes.
AC_DEFINE_INTEGER_BITS(int8_t, signed char, __int8, char)
AC_DEFINE_INTEGER_BITS(uint8_t, unsigned int8_t, unsigned char,
                       unsigned __int8, char)
AC_DEFINE_INTEGER_BITS(int16_t, signed short, signed int, __int16,
                       short, int)
AC_DEFINE_INTEGER_BITS(uint16_t, unsigned int16_t, unsigned short, unsigned,
                       unsigned __int16)
AC_DEFINE_INTEGER_BITS(int32_t, signed, signed long, __int32, int, long)
AC_DEFINE_INTEGER_BITS(uint32_t, unsigned int32_t, unsigned, unsigned long,
                       unsigned __int32)
AC_DEFINE_INTEGER_BITS(int64_t, signed long, signed long long,
                       __int64, long long)
AC_DEFINE_INTEGER_BITS(uint64_t, unsigned int64_t, unsigned long,
                       unsigned long long, unsigned __int64)

AC_CXX_TYPE_EQUIVALENCE(int32_t, int)

AS_IF([test "$uint64_t" = no],
  dnl Check for endianness
  [AC_C_BIGENDIAN()],
  [AC_DEFINE([HAVE_64BIT_TYPES], [1],
    [Defined when the compiler supports 64 bit integer math])])

dnl	Check the stat() system call to find out how many
dnl	underscores are needed in front of S_IFDIR

AC_CACHE_CHECK(for __S_IFDIR symbol,
    jikes_cv_stat_ifdir_2underscore,
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/stat.h>
]], [[int i = __S_IFDIR; i++;]])],[jikes_cv_stat_ifdir_2underscore=yes],[jikes_cv_stat_ifdir_2underscore=no]))

AC_CACHE_CHECK(for _S_IFDIR symbol,
    jikes_cv_stat_ifdir_1underscore,
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/stat.h>
]], [[int i = _S_IFDIR; i++;]])],[jikes_cv_stat_ifdir_1underscore=yes],[jikes_cv_stat_ifdir_1underscore=no]))

AC_CACHE_CHECK(for S_IFDIR symbol,
    jikes_cv_stat_ifdir_0underscore,
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/stat.h>
]], [[int i = S_IFDIR; i++;]])],[jikes_cv_stat_ifdir_0underscore=yes],[jikes_cv_stat_ifdir_0underscore=no]))

if test "$jikes_cv_stat_ifdir_2underscore" = "yes"; then
    AC_DEFINE(JIKES_STAT_S_IFDIR, __S_IFDIR,
    [Used to test results of stat call to see if it is a directory])
elif test "$jikes_cv_stat_ifdir_1underscore" = "yes"; then
    AC_DEFINE(JIKES_STAT_S_IFDIR, _S_IFDIR,
    [Used to test results of stat call to see if it is a directory])
elif test "$jikes_cv_stat_ifdir_0underscore" = "yes"; then
    AC_DEFINE(JIKES_STAT_S_IFDIR, S_IFDIR,
    [Used to test results of stat call to see if it is a directory])
else
    AC_MSG_ERROR([Could not find stat S_IFDIR symbol])
fi



dnl We need to do some nasty checks here to make sure that
dnl we know what version of mkdir() to call.

dnl First, we just make sure mkdir() actually exists
AC_CHECK_FUNCS(mkdir, , AC_MSG_ERROR([No mkdir() function found]))

AC_CACHE_CHECK(for mac style mkdir, jikes_cv_mac_mkdir,
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/stat.h>
#include <stat.mac.h>
]], [[mkdir("foo.dir", 0);
]])],[jikes_cv_mac_mkdir=yes],[jikes_cv_mac_mkdir=no])
)

AC_CACHE_CHECK(for glibc style mkdir, jikes_cv_glibc_mkdir,
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/stat.h>
#include <unistd.h>
]], [[mkdir("foo.dir", S_IRWXU | S_IRWXG | S_IRWXO);
]])],[jikes_cv_glibc_mkdir=yes],[jikes_cv_glibc_mkdir=no])
)

AC_CACHE_CHECK(for libc5 style mkdir, jikes_cv_libc5_mkdir,
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/stat.h>
#include <unistd.h>
]], [[mkdir("foo.dir", S_IRWXU);
]])],[jikes_cv_libc5_mkdir=yes],[jikes_cv_libc5_mkdir=no])
)

AC_CACHE_CHECK(for win32 style mkdir, jikes_cv_win32_mkdir,
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <direct.h>
]], [[mkdir("foo.dir");
]])],[jikes_cv_win32_mkdir=yes],[jikes_cv_win32_mkdir=no])
)

if test "$jikes_cv_glibc_mkdir" = "yes" ; then
    AC_DEFINE(HAVE_GLIBC_MKDIR, ,
        [use unix style mkdir(str, S_IRWXU | S_IRWXG | S_IRWXO)])
elif test "$jikes_cv_libc5_mkdir" = "yes" ; then
    AC_DEFINE(HAVE_LIBC5_MKDIR, ,
        [use unix style mkdir(str, S_IRWXU)])
elif test "$jikes_cv_win32_mkdir" = "yes" ; then
    AC_DEFINE(HAVE_WIN32_MKDIR, ,
        [use win32 style mkdir(str) from <direct.h>])
elif test "$jikes_cv_mac_mkdir" = "yes" ; then
    AC_DEFINE(HAVE_MAC_MKDIR, ,
        [use mac style mkdir(str,0) from <stat.mac.h>])
else
    AC_MSG_ERROR([Could not locate a working mkdir() implementation])
fi


dnl xlC (AIX), ICC (Win*, OS/2) and vacpp (all) are all compilers from
dnl IBM, they are related in that there is a common code base. On the
dnl x86 platform when compiling with ICC we need to set control registers
dnl on the math co-proc inorder to work around floating point oddities.

dnl FIXME: need to limit to x86 architecture.
dnl FIXME: case sensitive string compare to compiler name is BAD.
dnl FIXME: need a better test than "does it compile"...
dnl        need to acutally USE it. Ask Dave.

dnl old logic within the ICC/xlC shared config block (added by cabbey@home.net
dnl because it was setting this flag on PowerRS based AIX boxen)
dnl To really fix this, we need to find out what ${host_os} is set
dnl to when compiling on windows or OS2.

dnl Something like this ???
dnl case "${host_os}" in                                                       
dnl        *86)
dnl            jikes_cv_icc_fp_bugs=yes
dnl            ;;
dnl esac

AC_CACHE_CHECK([floating point bug workaround for ICC compiler],
               [jikes_cv_icc_fp_bugs],
   [AS_IF([test "$CXX" = ICC || test "$CXX" = vacpp],
          [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <float.h>]],
             [[ _control87(EM_UNDERFLOW, EM_UNDERFLOW); ]])],
             [jikes_cv_icc_fp_bugs=yes],[jikes_cv_icc_fp_bugs=no])],
          [jikes_cv_icc_fp_bugs=no])])

AS_IF([test "$jikes_cv_icc_fp_bugs" = yes],
    [AC_DEFINE([HAVE_ICC_FP_BUGS], [],
    [known to be broken on Win and OS2 when compiled with ICC])])

dnl	see if jikespg location is given on the command line
AC_ARG_WITH([jikespg], [  --with-jikespg=PROG     location of jikes parser generator (optional)],
    [JIKESPG=$withval], [JIKESPG=NONE])

AS_IF([test "$JIKESPG" != NONE],
      [AS_IF([test ! -f "$JIKESPG" || test ! -x "$JIKESPG"],
             [AC_MSG_ERROR([jikespg argument '$JIKESPG' is not a file or is not executable])])],
      [JIKESPG=jikespg])
AC_SUBST(JIKESPG)


# Turn on beta version of 1.5 support
AC_ARG_ENABLE([source15],
              [AS_HELP_STRING([--enable-source15],
                              [Enable experimental 1.5 source features])],
              [source15=$enableval], [source15=yes])
AS_IF([test $source15 = yes],
      [AC_DEFINE([ENABLE_SOURCE_15], [1],
       [Define as nonzero to enable experimental `-source 1.5' features.])],
      [AC_DEFINE([ENABLE_SOURCE_15], [0],
       [Define as nonzero to enable experimental `-source 1.5' features.])])

# Subst variables into Makefile.in to produce the Makefile

AC_CONFIG_FILES([Makefile doc/Makefile src/Makefile])
AC_OUTPUT
