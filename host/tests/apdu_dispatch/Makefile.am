AM_LDFLAGS = -no-install
AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include
AM_CFLAGS=-Wall $(LIBOSMONETIF_CFLAGS) $(LIBOSMOCORE_CFLAGS) $(LIBOSMOVTY_CFLAGS)
LDADD = $(top_builddir)/lib/.libs/libosmo-simtrace2.la \
    $(LIBOSMOCORE_LIBS) $(LIBOSMOGSM_LIBS) $(LIBOSMOVTY_LIBS) $(LIBOSMONETIF_LIBS) $(LIBSCTP_LIBS)

EXTRA_DIST = \
    apdu_dispatch_test.ok \
    $(NULL)

check_PROGRAMS = apdu_dispatch_test

apdu_dispatch_test_SOURCES = apdu_dispatch_test.c
