lib_LIBRARIES = libcomm_nrf24l01.a

libcomm_nrf24l01_a_SOURCES = comm_nrf24l01.c
libcomm_nrf24l01_a_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/nrf24l01

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs

clean-local:
	$(RM) -r libcomm_nrf24l01.a
	$(RM) -r $(top_srcdir)/libs
