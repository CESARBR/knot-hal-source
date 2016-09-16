lib_LIBRARIES = libhalstorage.a

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS) -I$(top_srcdir)/include

libhalstorage_a_SOURCES = storage_linux.c
libhalstorage_a_CPPFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs

clean-local:
	$(RM) -r libhalstorage.a

