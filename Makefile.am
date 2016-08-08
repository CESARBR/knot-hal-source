AM_MAKEFLAGS = --no-print-directory

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS) @GLIB_CFLAGS@
AM_LDFLAGS = $(BUILD_LDFLAGS)

libexec_PROGRAMS = proxy/spiproxyd nrf/nrfd

proxy_spiproxyd_SOURCES = proxy/main.c

proxy_spiproxyd_LDADD = @GLIB_LIBS@
proxy_spiproxyd_LDFLAGS = $(AM_LDFLAGS)
proxy_spiproxyd_CFLAGS = $(AM_CFLAGS)

nrf_nrfd_SOURCES = nrf/main.c
nrf_nrfd_LDADD = @GLIB_LIBS@
nrf_nrfd_LDFLAGS = $(AM_LDFLAGS)
nrf_nrfd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@

DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r proxy/spiproxyd nrf/nrfd
