# TODO: change this variables according to operating system
MKDIR=mkdir
CP=cp
ZIP=zip

AM_MAKEFLAGS = --no-print-directory
SUBDIRS = src/spi src/nrf24l01 src/hal/time src/hal/log \
					src/hal/comm src/drivers src/hal/gpio

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
AM_LDFLAGS = $(BUILD_LDFLAGS)

lib_headers =  $(top_srcdir)/hal/comm.h \
		$(top_srcdir)/hal/gpio.h \
		$(top_srcdir)/hal/gpio_sysfs.h \
		$(top_srcdir)/hal/linux_log.h \
		$(top_srcdir)/hal/nrf24.h \
		$(top_srcdir)/hal/time.h

extra_headers = $(top_srcdir)/hal/storage.h \
		 $(top_srcdir)/hal/avr_error.h \
		 $(top_srcdir)/hal/avr_log.h \
		 $(top_srcdir)/hal/avr_unistd.h


lib_LTLIBRARIES = libhal.la
libhal_la_LIBADD = $(top_srcdir)/src/hal/log/libhallog.la $(top_srcdir)/src/hal/gpio/libhalgpio.la \
		   $(top_srcdir)/src/hal/time/libhaltime.la $(top_srcdir)/src/spi/libspi.la \
		   $(top_srcdir)/src/drivers/libphy_driver.la $(top_srcdir)/src/nrf24l01/libnrf24l01.la

if SERIAL
libhal_la_LIBADD += $(top_srcdir)/src/hal/comm/libhalcommserial.la
else
libhal_la_LIBADD += $(top_srcdir)/src/hal/comm/libhalcommnrf24.la
endif

libhal_la_LDFLAGS = $(AM_LDFLAGS)
libhal_la_SOURCES = $(lib_headers)

libhal_includedir = $(includedir)/hal
libhal_include_HEADERS = $(libhal_la_SOURCES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/hal.pc

bin_PROGRAMS = proxy/spiproxyd src/lorad/lorad tools/rpiecho

if SERIAL
bin_PROGRAMS += src/seriald/seriald
else
bin_PROGRAMS += tools/sniffer
endif

proxy_spiproxyd_SOURCES = proxy/main.c
proxy_spiproxyd_LDADD = libhal.la @GLIB_LIBS@
proxy_spiproxyd_LDFLAGS = $(AM_LDFLAGS)
proxy_spiproxyd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@

src_lorad_lorad_SOURCES = src/lorad/main.c
src_lorad_lorad_LDADD = @GLIB_LIBS@
src_lorad_lorad_LDFLAGS = $(AM_LDFLAGS)
src_lorad_lorad_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@

if !SERIAL
tools_sniffer_SOURCES = tools/sniffer.c
tools_sniffer_LDADD = libhal.la \
				@GLIB_LIBS@
tools_sniffer_LDFLAGS = $(AM_LDFLAGS)
tools_sniffer_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
		-I$(top_srcdir)/src/drivers -I$(top_srcdir)/src/hal/comm \
		-I$(top_srcdir)/src/spi \
		-I$(top_srcdir)/src/hal/gpio_sysfs \
		-I$(top_srcdir)/src/nrf24l01
endif

tools_rpiecho_SOURCES = tools/rpiecho.c
tools_rpiecho_LDADD = libhal.la \
				 @GLIB_LIBS@
tools_rpiecho_LDFLAGS = $(AM_LDFLAGS)
tools_rpiecho_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/spi \
				-I$(top_srcdir)/src/hal/gpio_sysfs \
				-I$(top_srcdir)/src/nrf24l01 \
				-I$(top_srcdir)/nrf
if SERIAL
src_seriald_seriald_SOURCES = src/seriald/main.c \
				src/seriald/manager.h src/seriald/manager.c
src_seriald_seriald_LDADD = libhal.la @GLIB_LIBS@
src_seriald_seriald_LDFLAGS = $(AM_LDFLAGS)
src_seriald_seriald_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src
endif
DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r proxy/spiproxyd src/lorad/lorad tools/rpiecho tools/sniffer
