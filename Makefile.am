AM_MAKEFLAGS = --no-print-directory
SUBDIRS = src src/spi/ src/nrf24l01 src/hal/storage
AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
AM_LDFLAGS = $(BUILD_LDFLAGS)

bin_PROGRAMS = proxy/spiproxyd src/phynrfd/phynrfd src/phyemud/phyemud \
							examples/uthingd

proxy_spiproxyd_SOURCES = proxy/main.c
proxy_spiproxyd_LDADD = libs/libspi.a libs/libnrf24l01.a @GLIB_LIBS@
proxy_spiproxyd_LDFLAGS = $(AM_LDFLAGS)
proxy_spiproxyd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/spi \
				-I$(top_srcdir)/src/nrf24l01

src_phynrfd_phynrfd_SOURCES = src/phynrfd/main.c \
				src/phynrfd/manager.h src/phynrfd/manager.c
src_phynrfd_phynrfd_LDADD = libs/libcomm_nrf24l01.a \
				 libs/libnrf24l01.a libs/libspi.a \
				 @GLIB_LIBS@
src_phynrfd_phynrfd_LDFLAGS = $(AM_LDFLAGS)
src_phynrfd_phynrfd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/spi \
				-I$(top_srcdir)/src/nrf24l01 \
				-I$(top_srcdir)/nrf \
				-I$(top_srcdir)/include

src_phyemud_phyemud_SOURCES = src/phyemud/main.c \
			src/comm_private.h \
			src/phyemud/manager.h src/phyemud/manager.c \
			src/phyemud/phyunix.c src/phyemud/physerial.c
src_phyemud_phyemud_LDADD = @GLIB_LIBS@
src_phyemud_phyemud_LDFLAGS = $(AM_LDFLAGS)
src_phyemud_phyemud_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src

examples_uthingd_SOURCES = examples/uthing.c \
			src/comm_private.h \
			src/phyemud/phyunix.c
examples_uthingd_LDFLAGS = $(AM_LDFLAGS)
examples_uthingd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src

DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

clean-local:
	$(RM) -r proxy/spiproxyd nrf/nrfd
