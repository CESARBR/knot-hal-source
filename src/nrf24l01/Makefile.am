lib_LIBRARIES = libnrf24l01.a
lib_ARDUINO = nrf24l01.c nrf24l01.h nrf24l01_io.h nrf24l01_io_arduino.c

libnrf24l01_a_SOURCES = nrf24l01.c nrf24l01_io_linux.c crypto.c
libnrf24l01_a_CPPFLAGS = -I$(top_srcdir)/src/ -I$(top_srcdir)/src/spi

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/include/arduino && cp $(lib_ARDUINO) $(top_srcdir)/include/arduino

clean-local:
	$(RM) -r libnrf24l01.a


