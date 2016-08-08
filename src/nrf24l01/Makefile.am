lib_LIBRARIES = libnrf24l01.a

libnrf24l01_a_SOURCES = nrf24l01.c
libnrf24l01_a_CPPFLAGS = -I$(top_srcdir)/src/ -I$(top_srcdir)/src/spi

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs

clean-local:
	$(RM) -r libnrf24l01.a


