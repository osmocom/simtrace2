AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include
AM_CFLAGS=-Wall -g $(LIBOSMOCORE_CFLAGS) $(LIBOSMOSIM_CFLAGS) $(LIBUSB_CFLAGS) $(COVERAGE_FLAGS)
AM_LDFLAGS=$(COVERAGE_LDFLAGS)

LDADD= $(top_builddir)/lib/libosmo-simtrace2.la \
       $(LIBOSMOCORE_LIBS) $(LIBOSMOSIM_LIBS) $(LIBUSB_LIBS)

bin_PROGRAMS = simtrace2-remsim simtrace2-remsim-usb2udp simtrace2-list simtrace2-sniff

simtrace2_remsim_SOURCES = simtrace2-remsim.c

simtrace2_remsim_usb2udp_SOURCES = usb2udp.c

simtrace2_list_SOURCES = simtrace2_usb.c

simtrace2_sniff_SOURCES = simtrace2-sniff.c
