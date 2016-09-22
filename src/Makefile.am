lib_LIBRARIES = libcomm_nrf24l01.a
lib_ARDUINO = comm_nrf24l01.c comm_private.h \
				avr_errno.h avr_unistd.h

libcomm_nrf24l01_a_SOURCES = comm_nrf24l01.c
libcomm_nrf24l01_a_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/nrf24l01

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/include/arduino && cp $(lib_ARDUINO) $(top_srcdir)/include/arduino
clean-local:
	$(RM) -r libcomm_nrf24l01.a
	$(RM) -r $(top_srcdir)/libs
