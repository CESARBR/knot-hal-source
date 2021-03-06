AC_PREREQ(2.60)
AC_INIT(knot-hal, 0.01)

AM_INIT_AUTOMAKE([silent-rules subdir-objects])

AC_CONFIG_HEADERS(config.h)

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AM_MAINTAINER_MODE

AC_PREFIX_DEFAULT(/usr)

PKG_PROG_PKG_CONFIG

COMPILER_WARNING_CFLAGS
COMPILER_BUILD_CFLAGS

AC_LANG_C

AC_PROG_CC
AC_PROG_CC_PIE

m4_define([_LT_AC_TAGCONFIG], [])
m4_ifdef([AC_LIBTOOL_TAGS], [AC_LIBTOOL_TAGS([])])

AC_PROG_LIBTOOL
AC_ARG_WITH([network], AC_HELP_STRING([--with-network=ARG],
				[Type of network yield to the library]),
					[type_network=${withval}])
AC_MSG_CHECKING([for network type setting])
if (test -z "${type_network}"); then
	type_network="nrf24"
else
	if (test "${type_network}" != "nrf24" -a \
		"${type_network}" != "serial"); then
		AC_MSG_ERROR([No supported network])
	fi
fi
AC_MSG_RESULT([${type_network}])
AM_CONDITIONAL(SERIAL, test "${type_network}" = "serial")

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.52, dummy=no,
				AC_MSG_ERROR(required glib >= 2.52))
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GIO_UNIX, gio-2.0 >= 2.48)

AC_SUBST(GIO_UNIX_CFLAGS)
AC_SUBST(GIO_UNIX_LIBS)

PKG_CHECK_MODULES(JSON, json-c,
  [AC_DEFINE([HAVE_JSON],[1],[Use JSON])],
  [AC_MSG_ERROR("json-c missing")])
AC_SUBST(JSON_CFLAGS)
AC_SUBST(JSON_LIBS)

AC_CONFIG_FILES([Makefile src/spi/Makefile src/nrf24l01/Makefile \
		src/hal/time/Makefile src/hal/gpio/Makefile \
		src/hal/log/Makefile src/hal/comm/Makefile src/drivers/Makefile \
		src/hal.pc])
AC_OUTPUT
