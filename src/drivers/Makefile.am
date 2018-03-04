noinst_LTLIBRARIES = libphy_driver.la
lib_ARDUINO = phy_driver.c phy_driver_nrf24.c

AM_CFLAGS = $(WARNING_CFLAGS) $(BUILD_CFLAGS)

libphy_driver_la_SOURCES = phy_driver.c phy_driver_nrf24.c
libphy_driver_la_CPPFLAGS = $(AM_CFLAGS) -I$(top_srcdir)/src/hal/comm \
								-I$(top_srcdir)/src/nrf24l01

all-local:
	$(MKDIR_P) $(top_srcdir)/libs && cp $(noinst_LTLIBRARIES) .libs/libphy_driver.a $(top_srcdir)/libs
	$(MKDIR_P) $(top_srcdir)/hal/arduino && cp $(lib_ARDUINO) $(top_srcdir)/hal/arduino
clean-local:
	$(RM) -r libphy_driver.la
	$(RM) -r $(top_srcdir)/libs
