lib_LIBRARIES = libnrf24l01_driver_linux.a

libnrf24l01_driver_linux_a_SOURCES = nrf24l01_driver_linux.c
libnrf24l01_driver_linux_a_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/nrf24l01

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs

clean-local:
	$(RM) -r libnrf24l01_driver_linux.a
	$(RM) -r $(top_srcdir)/libs