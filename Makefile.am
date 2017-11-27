# TODO: change this variables according to operating system
MKDIR=mkdir
CP=cp
ZIP=zip

AM_MAKEFLAGS = --no-print-directory
SUBDIRS = src/spi src/nrf24l01 src/hal/storage src/hal/time src/hal/log \
					src/hal/comm src/drivers src/hal/gpio

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
AM_LDFLAGS = $(BUILD_LDFLAGS)

bin_PROGRAMS = proxy/spiproxyd src/nrfd/nrfd src/lorad/lorad tools/sniffer tools/rpiecho \
						src/seriald/seriald

proxy_spiproxyd_SOURCES = proxy/main.c
proxy_spiproxyd_LDADD = libs/libspi.a libs/libnrf24l01.a @GLIB_LIBS@
proxy_spiproxyd_LDFLAGS = $(AM_LDFLAGS)
proxy_spiproxyd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@

src_nrfd_nrfd_SOURCES = src/nrfd/main.c \
				src/nrfd/manager.h src/nrfd/manager.c
src_nrfd_nrfd_LDADD = libs/libhallog.a libs/libhalcommnrf24.a \
				libs/libphy_driver.a \
				libs/libhaltime.a \
				libs/libhalstorage.a \
				libs/libnrf24l01.a \
				libs/libspi.a \
				libs/libhalgpio.a \
				@GLIB_LIBS@ @JSON_LIBS@ \
				@GIO_UNIX_LIBS@
src_nrfd_nrfd_LDFLAGS = $(AM_LDFLAGS)
src_nrfd_nrfd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				@GIO_UNIX_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/nrf24l01 \
				-I$(top_srcdir)/src/drivers \
				-I$(top_srcdir)/src/hal/comm \
				-I$(top_srcdir)/src/hal/gpio_sysfs \
				-I$(top_srcdir)/src/nrf24l01 @JSON_CFLAGS@

src_lorad_lorad_SOURCES = src/lorad/main.c
src_lorad_lorad_LDADD = @GLIB_LIBS@
src_lorad_lorad_LDFLAGS = $(AM_LDFLAGS)
src_lorad_lorad_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@

tools_sniffer_SOURCES = tools/sniffer.c
tools_sniffer_LDADD = libs/libphy_driver.a \
				libs/libhalcommnrf24.a \
				libs/libhaltime.a \
				libs/libnrf24l01.a \
				libs/libspi.a \
				libs/libhallog.a \
				libs/libhalgpio.a \
				@GLIB_LIBS@
tools_sniffer_LDFLAGS = $(AM_LDFLAGS)
tools_sniffer_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
		-I$(top_srcdir)/src/drivers -I$(top_srcdir)/src/hal/comm \
		-I$(top_srcdir)/src/spi \
		-I$(top_srcdir)/src/hal/gpio_sysfs \
		-I$(top_srcdir)/src/nrf24l01

tools_rpiecho_SOURCES = tools/rpiecho.c
tools_rpiecho_LDADD = libs/libphy_driver.a \
				 libs/libnrf24l01.a libs/libspi.a \
				 libs/libhalgpio.a \
				 libs/libhaltime.a \
				 @GLIB_LIBS@
tools_rpiecho_LDFLAGS = $(AM_LDFLAGS)
tools_rpiecho_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/spi \
				-I$(top_srcdir)/src/hal/gpio_sysfs \
				-I$(top_srcdir)/src/nrf24l01 \
				-I$(top_srcdir)/nrf

src_seriald_seriald_SOURCES = src/seriald/main.c \
				src/seriald/manager.h src/seriald/manager.c
src_seriald_seriald_LDADD = libs/libhalcommserial.a @GLIB_LIBS@
src_seriald_seriald_LDFLAGS = $(AM_LDFLAGS)
src_seriald_seriald_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src

DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r proxy/spiproxyd src/nrfd/nrfd src/lorad/lorad tools/rpiecho tools/sniffer
