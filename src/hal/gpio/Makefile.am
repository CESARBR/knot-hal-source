lib_LIBRARIES = libhalgpio.a

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libhalgpio_a_SOURCES = gpio_sysfs.c
libhalgpio_a_DEPENDENCIES = $(top_srcdir)/hal/gpio_sysfs.h

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs

clean-local:
	$(RM) -r libhalgpio.a
