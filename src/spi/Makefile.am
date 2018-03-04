noinst_LTLIBRARIES = libspi.la
AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
lib_ARDUINO = spi_arduino.cpp spi_bus.h

libspi_la_SOURCES = spi_linux.c
libspi_la_CPPFLAGS = -I$(top_srcdir)/src

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(noinst_LTLIBRARIES) .libs/libspi.a $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/hal/arduino && cp $(lib_ARDUINO) $(top_srcdir)/hal/arduino

clean-local:
	$(RM) -r libspi.la
