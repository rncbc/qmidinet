#ifndef CONFIG_H
#define CONFIG_H

/* Define to the title of this package. */
#cmakedefine PROJECT_TITLE "@PROJECT_TITLE@"

/* Define to the name of this package. */
#cmakedefine PROJECT_NAME "@PROJECT_NAME@"

/* Define to the version of this package. */
#cmakedefine PROJECT_VERSION "@PROJECT_VERSION@"

/* Define to the description of this package. */
#cmakedefine PROJECT_DESCRIPTION "@PROJECT_DESCRIPTION@"

/* Define to the homepage of this package. */
#cmakedefine PROJECT_HOMEPAGE_URL "@PROJECT_HOMEPAGE_URL@"

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

/* Define if IPv6 is supported */
#cmakedefine CONFIG_IPV6 @CONFIG_IPV6@

/* Define if Unique/Single instance is enabled. */
#cmakedefine CONFIG_XUNIQUE @CONFIG_XUNIQUE@

/* Define if Wayland is supported */
#cmakedefine CONFIG_WAYLAND @CONFIG_WAYLAND@

#endif /* CONFIG_H */
