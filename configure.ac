AC_INIT([Find Four], [0.9.2], [], [findfour])

AC_REVISION([Revision 5])

AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_SRCDIR([src/findfour.c])

# For debian systems, /usr as default
AC_PREFIX_DEFAULT([/usr])

# We need Gcc
AC_PROG_CC
AC_PROG_OBJC

# and automake
AM_INIT_AUTOMAKE([-Wall -Werror])

# Translate this program
AM_GNU_GETTEXT_VERSION([0.19.3])
AM_GNU_GETTEXT([external])

ALL_LINGUAS="es"
AC_SUBST(ALL_LINGUAS)

AM_PROG_CC_C_O

# We need to know if we have a working version of fork
AC_ARG_ENABLE(fork-resolv,
AC_HELP_STRING([--enable-fork-resolv],
	       [enable support for async resolv names using fork (default is YES)]),
[case "${enableval}" in
  yes) enable_fork_resolv=true ;;
  no)  enable_fork_resolv=false ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for --enable-fork-resolv) ;;
esac], [enable_fork_resolv=true])

if test "xtrue" = x"$enable_fork_resolv"; then
	AC_FUNC_FORK
fi

# Revisar el host
AC_CANONICAL_HOST

case $host_os in
  *mingw32* ) MINGW32=yes;;
          * ) MINGW32=no;;
esac
case $host_os in
  *cygwin* ) CYGWIN=yes;;
         * ) CYGWIN=no;;
esac
case $host_os in
  *linux* ) LINUX=yes;;
        * ) LINUX=no;;
esac
case $host_os in
 *darwin* ) MACOSX=yes;;
       * ) MACOSX=no;;
esac

AM_CONDITIONAL(MINGW32, test x$MINGW32 = xyes)
AM_CONDITIONAL(LINUX, test x$LINUX = xyes)
AM_CONDITIONAL(MACOSX, test x$MACOSX = xyes)

# Check for pkg-config
if test "x$MACOSX" = xno; then
dnl En Mac no es necesario el pkg-config, porque se usan los frameworks
	PKG_PROG_PKG_CONFIG
fi

SDL_VERSION=1.2.14
SDL_IMAGE_VERSION=1.2.10
SDL_TTF_VERSION=2.0.11
SDL_MIXER_VERSION=1.2.12

if test "x$MACOSX" = xyes; then
	AC_MSG_WARN([*******************************************************])
	AC_MSG_WARN([Cannot check if SDL librarys are installed on MAC OS X.])
	AC_MSG_WARN([The following code assumes that the Frameworks are installed on the system locations])
	AC_MSG_WARN([If they are not, use the --with-frameworks option])
	AC_MSG_WARN([*******************************************************])
	
	SDL_FRAMEWORKS="SDL SDL_image SDL_mixer SDL_ttf"
	
	for g in $SDL_FRAMEWORKS; do
		AC_LIB_APPENDTOVAR([CPPFLAGS], [-I/Library/Frameworks/$g.framework/Headers])
	done
	AC_ARG_WITH([frameworks], [AS_HELP_STRING(
		[--with-frameworks=DIRS],
		[add to DIRS (space separated) to the Darwin include path for frameworks with -F]
	)], [
		for cy_framework in $withval; do
			AC_LIB_APPENDTOVAR([CFLAGS], [-F$cy_framework])
			AC_LIB_APPENDTOVAR([CXXFLAGS], [-F$cy_framework])
			AC_LIB_APPENDTOVAR([OBJCCXXFLAGS], [-F$cy_framework])
			AC_LIB_APPENDTOVAR([LDFLAGS], [-F$cy_framework])
			for g in $SDL_FRAMEWORKS; do
				AC_LIB_APPENDTOVAR([CPPFLAGS], [-I$cy_framework/$g.framework/Headers])
			done
		done
	])
	
	AC_LIB_APPENDTOVAR([LIBS], ["-framework AudioToolbox -framework AudioUnit -framework Cocoa -framework CoreAudio -framework IOKit -framework CoreFoundation -framework Carbon -framework CoreServices -framework ApplicationServices -framework Foundation -framework AppKit -framework OpenGL -framework SDL -framework SDL_image -framework SDL_mixer -framework SDL_ttf"])
else
	AC_MSG_CHECKING([if you have SDL installed on your system])
	PKG_CHECK_EXISTS([sdl >= $SDL_VERSION], [AC_MSG_RESULT([yes])], [AC_MSG_FAILURE([SDL not found in your system])])
	PKG_CHECK_MODULES(SDL, [sdl >= $SDL_VERSION], [], [])

	AC_MSG_CHECKING([if you have SDL_image installed on your system])
	PKG_CHECK_EXISTS([SDL_image >= $SDL_IMAGE_VERSION], [AC_MSG_RESULT([yes])], [AC_MSG_FAILURE([SDL_image not found in your system])])
	PKG_CHECK_MODULES(SDL_image, [SDL_image >= $SDL_IMAGE_VERSION], [], [])

	AC_MSG_CHECKING([if you have SDL_ttf installed on your system])
	PKG_CHECK_EXISTS([SDL_ttf >= $SDL_TTF_VERSION], [AC_MSG_RESULT([yes])], [AC_MSG_FAILURE([SDL_ttf not found in your system])])
	PKG_CHECK_MODULES(SDL_ttf, [SDL_ttf >= $SDL_TTF_VERSION], [], [])

	AC_MSG_CHECKING([if you have SDL_mixer installed on your system])
	PKG_CHECK_EXISTS([SDL_mixer >= $SDL_MIXER_VERSION], [AC_MSG_RESULT([yes])], [AC_MSG_FAILURE([SDL_mixer not found in your system])])
	PKG_CHECK_MODULES(SDL_mixer, [SDL_mixer >= $SDL_MIXER_VERSION], [], [])
fi
AC_CONFIG_HEADERS([config.h])

AC_CHECK_TOOL(WINDRES, windres)

# Checar si getifaddrs existe
AC_CHECK_FUNCS(getifaddrs)

dnl Add -DMACOSX to CXXFLAGS and CFLAGS if working under darwin
if test "x$MACOSX" = xyes; then
 CPPFLAGS="$CPPFLAGS -DMACOSX"
fi

AC_CONFIG_FILES([
                 Makefile
                 src/Makefile
                 data/Makefile
                 po/Makefile.in
                 etc/Makefile
                 etc/Info.plist
])

AC_OUTPUT
