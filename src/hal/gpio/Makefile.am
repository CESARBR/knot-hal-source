noinst_LTLIBRARIES = libhalgpio.la

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)
AM_LDFLAGS = $(BUILD_LDFLAGS)

libhalgpio_la_SOURCES = gpio_sysfs.c
libhalgpio_la_DEPENDENCIES = $(top_srcdir)/hal/gpio_sysfs.h

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(noinst_LTLIBRARIES) .libs/libhalgpio.a $(top_srcdir)/libs

clean-local:
	$(RM) -r libhalgpio.la
