noinst_LTLIBRARIES = libhalstorage.la

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libhalstorage_la_SOURCES = storage_linux.c
libhalstorage_la_CPPFLAGS = $(AM_CFLAGS) @GLIB_CFLAGS@
libhalstorage_la_DEPENDENCIES = $(top_srcdir)/hal/storage.h

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(noinst_LTLIBRARIES) .libs/libhalstorage.a $(top_srcdir)/libs

clean-local:
	$(RM) -r libhalstorage.la

