# TODO: change this variables according to operating system
MKDIR=mkdir
CP=cp
ZIP=zip

HAL = ./hal
HEADERS_DIR = ./include
HAL_DIR = ./src/hal
HAL_TARGET = hal.zip

AM_MAKEFLAGS = --no-print-directory
SUBDIRS = src/spi src/nrf24l01 src/hal/storage src/hal/time src/hal/log \
					src/hal/comm src/drivers

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
AM_LDFLAGS = $(BUILD_LDFLAGS)

bin_PROGRAMS = proxy/spiproxyd src/nrfd/nrfd

proxy_spiproxyd_SOURCES = proxy/main.c
proxy_spiproxyd_LDADD = libs/libspi.a libs/libnrf24l01.a @GLIB_LIBS@
proxy_spiproxyd_LDFLAGS = $(AM_LDFLAGS)
proxy_spiproxyd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@

src_nrfd_nrfd_SOURCES = src/nrfd/main.c \
				src/nrfd/manager.h src/nrfd/manager.c
src_nrfd_nrfd_LDADD = libs/libhallog.a \
				libs/libhalcommnrf24.a \
				libs/libhaltime.a \
				libs/libhalstorage.a \
				libs/libnrf24l01.a \
				libs/libspi.a \
				@GLIB_LIBS@ @JSON_LIBS@
src_nrfd_nrfd_LDFLAGS = $(AM_LDFLAGS)
src_nrfd_nrfd_CFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@ \
				-I$(top_srcdir)/src \
				-I$(top_srcdir)/src/nrf24l01 \
				-I$(top_srcdir)/src/drivers \
				-I$(top_srcdir)/src/hal/comm \
				-I$(top_srcdir)/src/nrf24l01 @JSON_CFLAGS@

DISTCLEANFILES =

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp compile missing install-sh

# TODO: add comm implementation
arduino-lib:
	$(MKDIR) -p $(HAL)

	$(CP) $(HEADERS_DIR)/*.h $(HAL)

	$(CP) $(HAL_DIR)/log/*cpp $(HAL)
	$(CP) $(HAL_DIR)/storage/*cpp $(HAL)
	$(CP) $(HAL_DIR)/time/*cpp $(HAL)

	$(ZIP) -r $(HAL_TARGET) $(HAL)

	$(RM) -r $(HAL)

clean-local:
	$(RM) -r proxy/spiproxyd nrf/nrfd $(HAL_TARGET)
