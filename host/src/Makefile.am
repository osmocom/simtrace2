AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include
AM_CFLAGS=-Wall -g $(LIBOSMOCORE_CFLAGS) $(LIBOSMOSIM_CFLAGS) $(LIBOSMOUSB_CFLAGS) $(LIBUSB_CFLAGS) $(COVERAGE_FLAGS)
AM_LDFLAGS=$(COVERAGE_LDFLAGS)

LDADD= $(top_builddir)/lib/libosmo-simtrace2.la \
       $(LIBOSMOCORE_LIBS) $(LIBOSMOSIM_LIBS) $(LIBOSMOUSB_LIBS) $(LIBUSB_LIBS)

bin_PROGRAMS = simtrace2-cardem-pcsc simtrace2-list simtrace2-sniff

simtrace2_cardem_pcsc_SOURCES = simtrace2-cardem-pcsc.c

simtrace2_list_SOURCES = simtrace2_usb.c

simtrace2_sniff_SOURCES = simtrace2-sniff.c
