lib_LIBRARIES = libhalgpio.a

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libhalgpio_a_SOURCES = gpio_linux.c
libhalgpio_a_DEPENDENCIES = $(top_srcdir)/hal/gpio.h

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(lib_LIBRARIES) $(top_srcdir)/libs

clean-local:
	$(RM) -r libhalgpio.a
