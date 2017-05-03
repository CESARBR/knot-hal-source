lib_LIBRARIES = libhaltime.a

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libhaltime_a_SOURCES = time_linux.c
libhaltime_a_CPPFLAGS = $(AM_CFLAGS)
libhaltime_a_DEPENDENCIES = $(top_srcdir)/hal/time.h

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs

clean-local:
	$(RM) -r libhaltime.a
