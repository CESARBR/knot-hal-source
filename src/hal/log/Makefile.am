lib_LIBRARIES = libhallog.a

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libhallog_a_SOURCES = log_linux.c
libhallog_a_DEPENDENCIES = $(top_srcdir)/include/linux_log.h

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs

clean-local:
	$(RM) -r libhallog.a
