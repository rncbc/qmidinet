# Process this file with autoconf to produce a configure script.
AC_INIT(QmidiNet, 0.3.0.4, rncbc@rncbc.org, qmidinet)

AC_CONFIG_SRCDIR(src/qmidinet.cpp)
AC_CONFIG_HEADERS(src/config.h)
AC_CONFIG_FILES(Makefile qmidinet.spec src/src.pri src/qmidinet.desktop)

# Set default installation prefix.
AC_PREFIX_DEFAULT(/usr/local)
ac_prefix=$prefix
if test "x$ac_prefix" = "xNONE"; then
   ac_prefix=$ac_default_prefix
fi
AC_SUBST(ac_prefix)
AC_DEFINE_UNQUOTED(CONFIG_PREFIX, ["$ac_prefix"], [Default installation prefix.])

# Enable debugging argument option.
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug], [enable debugging (default=no)]),
  [ac_debug="$enableval"])

# Enable ALSA MIDI support option.
AC_ARG_ENABLE(alsa_midi,
  AC_HELP_STRING([--enable-alsa-midi], [enable ALSA MIDI support (default=yes)]),
  [ac_alsa_midi="$enableval"],
  [ac_alsa_midi="yes"])

# Enable JACK MIDI support option.
AC_ARG_ENABLE(jack_midi,
  AC_HELP_STRING([--enable-jack-midi], [enable JACK MIDI support (default=yes)]),
  [ac_jack_midi="$enableval"],
  [ac_jack_midi="yes"])

if test "x$ac_debug" = "xyes"; then
   AC_DEFINE(CONFIG_DEBUG, 1, [Define if debugging is enabled.])
   ac_debug="debug"
else
   ac_debug="release"
fi

AC_SUBST(ac_debug)


# Enable Qt4/5 availability.
AC_ARG_ENABLE(qt4,
  AC_HELP_STRING([--enable-qt4], [enable Qt4 build (default=no)]),
  [ac_qt4="$enableval"],
  [ac_qt4="no"])

AC_ARG_ENABLE(qt5,
  AC_HELP_STRING([--enable-qt5], [enable Qt5 build (default=yes)]),
  [ac_qt5="$enableval"],
  [ac_qt5="yes"])



# Standard installation base dirs.
ac_with_paths="/usr /usr/local"

# Set for alternate Qt4/5 installation dir.
AC_ARG_WITH(qt4,
  AC_HELP_STRING([--with-qt4=PATH], [use alternate Qt4 install path]),
  [ac_qt4_path="$withval"], [ac_qt4_path="no"])

AC_ARG_WITH(qt5,
  AC_HELP_STRING([--with-qt5=PATH], [use alternate Qt5 install path]),
  [ac_qt5_path="$withval"], [ac_qt5_path="no"])

if test "x$ac_qt4_path" != "xno"; then
   ac_with_paths="$ac_with_paths $ac_qt4_path"
   ac_qt4="yes"
fi

if test "x$ac_qt5_path" != "xno"; then
   ac_with_paths="$ac_with_paths $ac_qt5_path"
   ac_qt5="yes"
fi

if test "x$ac_qt5" = "xno"; then
   ac_qt4="yes"
fi
if test "x$ac_qt4" = "xyes"; then
   ac_qt5="no"
fi

# Set for alternate ALSA installation dir.
AC_ARG_WITH(alsa,
  AC_HELP_STRING([--with-alsa=PATH], [use alternate ALSA install path]),
  [ac_with_paths="$ac_with_paths $withval"])

# Set for alternate JACK installation dir.
AC_ARG_WITH(jack,
  AC_HELP_STRING([--with-jack=PATH], [use alternate JACK install path]),
  [ac_with_paths="$ac_with_paths $withval"])


# Honor user specified flags.
ac_cflags=$CFLAGS
ac_ldflags=$LDFLAGS


# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_GCC_TRADITIONAL

# Checks for languages.
AC_LANG_C
AC_LANG_CPLUSPLUS


# Check for proper flags.
ac_arch=`uname -m`

# Check for install paths and alternatives...
ac_libdirs="lib"

if test "x$ac_arch" = "xx86_64"; then
   ac_libdirs="$ac_libdirs lib64"
fi

CFLAGS="-fPIC $CFLAGS"
CPPFLAGS="-fPIC $CPPFLAGS"

# Prepend alternate dependencies paths.
ac_path=$PATH

for X in $ac_with_paths; do
  if test -d $X/bin; then
    ac_path="$X/bin:$ac_path"
  fi
  if test -d $X/include; then
    CFLAGS="-I$X/include $CFLAGS "
    CPPFLAGS="-I$X/include $CPPFLAGS"
    ac_incpath="$X/include $ac_incpath"
  fi
  for Y in $ac_libdirs; do
    if test -d $X/$Y; then
       LIBS="-L$X/$Y $LIBS"
       ac_libs="-L$X/$Y $ac_libs"
    fi
  done
done

# A common error message:
ac_errmsg="not found in current PATH. Maybe QT development environment isn't available."

if test "x$ac_qt4" = "xyes"; then
   AC_PATH_PROG(ac_qmake, qmake-qt4, [no], $ac_path)
fi
if test "x$ac_qt5" = "xyes"; then
   AC_PATH_PROG(ac_qmake, qmake-qt5, [no], $ac_path)
fi
if test "x$ac_qmake" = "xno"; then
   AC_PATH_PROG(ac_cv_qmake, qmake, [no], $ac_path)
   ac_qmake=$ac_cv_qmake
fi
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_errmsg])
fi

# Check for proper Qt4/5 major version.
ac_qt_version_major=`$ac_qmake -query QT_VERSION | cut -d. -f1`
if test "x$ac_qt4" = "xyes" -a "x$ac_qt_version_major" != "x4"; then
   AC_MSG_ERROR([qmake-qt4 $ac_errmsg (qt4-devel)])
fi
if test "x$ac_qt5" = "xyes" -a "x$ac_qt_version_major" != "x5"; then
   AC_MSG_ERROR([qmake-qt5 $ac_errmsg (qt5-devel)])
fi

ac_qt_install_bins=`$ac_qmake -query QT_INSTALL_BINS`
if test -d $ac_qt_install_bins; then
   ac_path="$ac_qt_install_bins:$ac_path"
fi

# Check it again, now with updated PATH, just in case...
AC_PATH_PROG(ac_cv_qmake, qmake, [no], $ac_path)
ac_qmake=$ac_cv_qmake
if test "x$ac_qmake" = "xno"; then
   AC_MSG_ERROR([qmake $ac_errmsg])
fi

AC_SUBST(ac_qmake)

ac_qt_install_headers=`$ac_qmake -query QT_INSTALL_HEADERS`
if test -d $ac_qt_install_headers; then
   CFLAGS="-I$ac_qt_install_headers $CFLAGS "
   CPPFLAGS="-I$ac_qt_install_headers $CPPFLAGS"
   ac_incpath="$ac_qt_install_headers $ac_incpath"
fi

ac_qt_install_libs=`$ac_qmake -query QT_INSTALL_LIBS`
if test -d $ac_qt_install_libs; then
   LIBS="-L$ac_qt_install_libs $LIBS"
   ac_libs="-L$ac_qt_install_libs $ac_libs"
fi

# Check for proper Qt4/5 version.
if test "x$ac_qt4" = "xyes"; then
   AC_CACHE_CHECK([for Qt library version >= 4.4],
      ac_cv_qtversion, [
      AC_TRY_COMPILE([#include "QtCore/qglobal.h"], [
         #if QT_VERSION < 0x040400 || QT_VERSION >= 0x050000
         #error Qt library 4.4 or greater required.
         #endif
      ], ac_cv_qtversion="yes", [
         echo "no; Qt 4.4 or greater is required"
         exit 1
      ])
   ])
fi
 
if test "x$ac_qt5" = "xyes"; then
   AC_CACHE_CHECK([for Qt library version >= 5.1],
      ac_cv_qtversion, [
      AC_TRY_COMPILE([#include "QtCore/qglobal.h"], [
         #if QT_VERSION < 0x050100 || QT_VERSION >= 0x060000
         #error Qt library 5.1 or greater required.
         #endif
      ], ac_cv_qtversion="yes", [
         echo "no; Qt 5.1 or greater is required"
         exit 1
      ])
   ])
fi

# Check for Qt moc utility.
AC_PATH_PROG(ac_moc, moc, [no], $ac_path)
if test "x$ac_moc" = "xno"; then
   AC_MSG_ERROR([moc $ac_errmsg])
fi
AC_SUBST(ac_moc)

# Check for Qt uic utility.
AC_PATH_PROG(ac_uic, uic, [no], $ac_path)
if test "x$ac_uic" = "xno"; then
   AC_MSG_ERROR([uic $ac_errmsg])
fi
AC_SUBST(ac_uic)

# Check for Qt lupdate utility.
AC_PATH_PROG(ac_lupdate, lupdate, [no], $ac_path)
if test "x$ac_lupdate" = "xno"; then
   AC_MSG_ERROR([lupdate $ac_errmsg])
fi
AC_SUBST(ac_lupdate)

# Check for Qt lrelease utility.
AC_PATH_PROG(ac_lrelease, lrelease, [no], $ac_path)
if test "x$ac_release" = "xno"; then
   AC_MSG_ERROR([lrelease $ac_errmsg])
fi
AC_SUBST(ac_lrelease)


# Checks for libraries.
AC_CHECK_LIB(m, main)
AC_CHECK_LIB(X11, main)
AC_CHECK_LIB(Xext, main)

# Check for ALSA libraries.
if test "x$ac_alsa_midi" = "xyes"; then
   AC_CHECK_LIB(asound, main, [ac_alsa_midi="yes"], [ac_alsa_midi="no"])
   if test "x$ac_alsa_midi" = "xno"; then
      AC_MSG_WARN([*** ALSA library not found.])
   else
      ac_libs="$ac_libs -lasound"
   fi
fi

# Check for JACK libraries.
if test "x$ac_jack_midi" = "xyes"; then
   AC_CHECK_LIB(jack, main, [ac_jack_midi="yes"], [ac_jack_midi="no"])
   if test "x$ac_jack_midi" = "xno"; then
      AC_MSG_WARN([*** JACK library not found.])
   else
      ac_libs="$ac_libs -ljack"
   fi
fi

AC_SUBST(ac_libs)


# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h sys/ioctl.h sys/stat.h unistd.h signal.h)

# Check for ALSA headers.
if test "x$ac_alsa_midi" = "xyes"; then
   AC_CHECK_HEADER(alsa/asoundlib.h, [ac_alsa_midi="yes"], [ac_alsa_midi="no"])
   if test "x$ac_alsa_midi" = "xno"; then
      AC_MSG_WARN([*** ALSA headers not found.])
   else
      AC_DEFINE(CONFIG_ALSA_MIDI, 1, [Define if ALSA MIDI support is enabled.])
   fi
fi

# Check for JACK headers.
if test "x$ac_jack_midi" = "xyes"; then
   AC_CHECK_HEADERS(jack/jack.h jack/midiport.h, [ac_jack_midi="yes"], [ac_jack_midi="no"])
   if test "x$ac_jack_midi" = "xno"; then
      AC_MSG_WARN([*** JACK headers not found.])
   else
      AC_DEFINE(CONFIG_JACK_MIDI, 1, [Define if JACK MIDI support is enabled.])
   fi
fi

AC_SUBST(ac_incpath)
AC_SUBST(ac_cflags)
AC_SUBST(ac_ldflags)


# Checks for typedefs, structures, and compiler characteristics.
# AC_C_CONST

# Finally produce a configure header file and the makefiles.
AC_OUTPUT

# make clean > /dev/null 2>&1

# Output summary message

echo
echo "  $PACKAGE_NAME $PACKAGE_VERSION"
echo
echo "  Build target . . . . . . . . . . . . . . . . . . .: $ac_debug"
echo
echo "  ALSA MIDI support  . . . . . . . . . . . . . . . .: $ac_alsa_midi"
echo "  JACK MIDI support  . . . . . . . . . . . . . . . .: $ac_jack_midi"
echo
echo "  Install prefix . . . . . . . . . . . . . . . . . .: $ac_prefix"
echo
echo "Now type 'make', followed by 'make install' as root."
echo

