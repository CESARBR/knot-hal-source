noinst_LTLIBRARIES = libhaltime.la

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libhaltime_la_SOURCES = time_linux.c
libhaltime_la_CPPFLAGS = $(AM_CFLAGS)
libhaltime_la_DEPENDENCIES = $(top_srcdir)/hal/time.h

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(noinst_LTLIBRARIES) .libs/libhaltime.a $(top_srcdir)/libs

clean-local:
	$(RM) -r libhaltime.la
