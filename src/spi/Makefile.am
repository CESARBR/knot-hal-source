lib_LIBRARIES = libspi.a
AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
lib_ARDUINO = spi_arduino.cpp spi.h

libspi_a_SOURCES = spi_linux.c
libspi_a_CPPFLAGS = -I$(top_srcdir)/src

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/hal/arduino && cp $(lib_ARDUINO) $(top_srcdir)/hal/arduino

clean-local:
	$(RM) -r libspi.a
