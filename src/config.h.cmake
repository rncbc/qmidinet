#ifndef CONFIG_H
#define CONFIG_H

/* Define to the full name of this package. */
#cmakedefine PACKAGE_NAME "@PACKAGE_NAME@"

/* Define to the full name and version of this package. */
#cmakedefine PACKAGE_STRING "@PACKAGE_STRING@"

/* Define to the version of this package. */
#cmakedefine PACKAGE_VERSION "@PACKAGE_VERSION@"

/* Define to the address where bug reports for this package should be sent. */
#cmakedefine PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"

/* Define to the one symbol short name of this package. */
#cmakedefine PACKAGE_TARNAME "@PACKAGE_TARNAME@"

/* Define to the version of this package. */
#cmakedefine CONFIG_VERSION "@CONFIG_VERSION@"

/* Define to the build version of this package. */
#cmakedefine CONFIG_BUILD_VERSION "@CONFIG_BUILD_VERSION@"

/* Default installation prefix. */
#cmakedefine CONFIG_PREFIX "@CONFIG_PREFIX@"

/* Define to target installation dirs. */
#cmakedefine CONFIG_BINDIR "@CONFIG_BINDIR@"
#cmakedefine CONFIG_LIBDIR "@CONFIG_LIBDIR@"
#cmakedefine CONFIG_DATADIR "@CONFIG_DATADIR@"
#cmakedefine CONFIG_MANDIR "@CONFIG_MANDIR@"

/* Define if debugging is enabled. */
#cmakedefine CONFIG_DEBUG @CONFIG_DEBUG@

/* Define to 1 if you have the <signal.h> header file. */
#cmakedefine HAVE_SIGNAL_H @HAVE_SIGNAL_H@

/* Define if debugging is enabled. */
#cmakedefine CONFIG_DEBUG @CONFIG_DEBUG@

/* Define if ALSA library is available. */
#cmakedefine CONFIG_ALSA_MIDI @CONFIG_ALSA_MIDI@

/* Define if JACK MIDI support is available. */
#cmakedefine CONFIG_JACK_MIDI @CONFIG_JACK_MIDI@

/* Define if IPv6 is supported */
#cmakedefine CONFIG_IPV6 @CONFIG_IPV6@

#endif /* CONFIG_H */
