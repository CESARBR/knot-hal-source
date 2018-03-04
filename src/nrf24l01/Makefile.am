noinst_LTLIBRARIES = libnrf24l01.la
AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
lib_ARDUINO = nrf24l01.c nrf24l01.h nrf24l01_io.h nrf24l01_io_arduino.c

libnrf24l01_la_SOURCES = nrf24l01.c nrf24l01_io_linux.c
libnrf24l01_la_CPPFLAGS = -I$(top_srcdir)/src/ -I$(top_srcdir)/src/spi \
						-I$(top_srcdir)/src/hal/gpio

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(noinst_LTLIBRARIES) .libs/libnrf24l01.a $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/hal/arduino && cp $(lib_ARDUINO) $(top_srcdir)/hal/arduino

clean-local:
	$(RM) -r libnrf24l01.la
