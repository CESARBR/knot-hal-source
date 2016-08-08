AM_MAKEFLAGS = --no-print-directory
SUBDIRS = src/spi/ src/nrf24l01
AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
AM_LDFLAGS = $(BUILD_LDFLAGS)

libexec_PROGRAMS = proxy/spiproxyd nrf/nrfd

proxy_spiproxyd_SOURCES = proxy/main.c
proxy_spiproxyd_LDADD = libs/libspi.a libs/libnrf24l01.a @GLIB_LIBS@
proxy_spiproxyd_LDFLAGS = $(AM_LDFLAGS)
proxy_spiproxyd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/spi \
				-I$(top_srcdir)/src/nrf24l01

nrf_nrfd_SOURCES = nrf/main.c
nrf_nrfd_LDADD = libs/libspi.a libs/libnrf24l01.a @GLIB_LIBS@
nrf_nrfd_LDFLAGS = $(AM_LDFLAGS)
nrf_nrfd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src/spi \
				-I$(top_srcdir)/src/nrf24l01

DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r proxy/spiproxyd nrf/nrfd
