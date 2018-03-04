noinst_LTLIBRARIES = libhallog.la

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libhallog_la_SOURCES = log_linux.c
libhallog_la_DEPENDENCIES = $(top_srcdir)/hal/linux_log.h

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(noinst_LTLIBRARIES) \
						.libs/libhallog.a  $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/libs/include && cp $(libhallog_la_DEPENDENCIES) \
							$(top_srcdir)/libs/include

clean-local:
	$(RM) -r libhallog.la
