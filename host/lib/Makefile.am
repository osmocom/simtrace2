# This is _NOT_ the library release version, it's an API version.
# Please read chapter "Library interface versions" of the libtool documentation
# before making any modifications: https://www.gnu.org/software/libtool/manual/html_node/Versioning.html
ST2_LIBVERSION=0:0:0

AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include -I$(top_builddir)
AM_CFLAGS= -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMOSIM_CFLAGS) $(LIBUSB_CFLAGS) $(COVERAGE_CFLAGS)
AM_LDFLAGS = $(COVERAGE_LDFLAGS)
COMMONLIBS = $(LIBOSMOCORE_LIBS) $(LIBOSMOSIM_LIBS) $(LIBUSB_LIBS)

lib_LTLIBRARIES = libosmo-simtrace2.la

libosmo_simtrace2_la_LDFLAGS = $(AM_LDFLAGS) -version-info $(ST2_LIBVERSION)
libosmo_simtrace2_la_LIBADD = $(COMMONLIBS)
libosmo_simtrace2_la_SOURCES = \
	apdu_dispatch.c \
	gsmtap.c \
	simtrace2_api.c \
	usb_util.c \
	$(NULL)
