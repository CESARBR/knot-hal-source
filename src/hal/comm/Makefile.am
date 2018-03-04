noinst_LTLIBRARIES = libhalcommnrf24.la libhalcommserial.la

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libhalcommnrf24_la_SOURCES = comm_nrf24l01.c
libhalcommnrf24_la_CPPFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/src/nrf24l01 \
					-I$(top_srcdir)/src/drivers
libhalcommnrf24_la_DEPENDENCIES = $(top_srcdir)/hal/comm.h

libhalcommserial_la_SOURCES = comm_serial_linux.c
libhalcommserial_la_CPPFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/src/drivers
libhalcommserial_la_DEPENDENCIES = $(top_srcdir)/hal/comm.h

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(noinst_LTLIBRARIES) \
						.libs/libhalcommserial.a \
						.libs/libhalcommnrf24.a $(top_srcdir)/libs

clean-local:
	$(RM) -r libhalcommnrf24.la
	$(RM) -r libhalcommserial.la
