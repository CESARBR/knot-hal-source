lib_LIBRARIES = libspi.a

libspi_a_SOURCES = spi_linux.c
libspi_a_CPPFLAGS = -I$(top_srcdir)/src

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs

clean-local:
	$(RM) -r libspi.a


