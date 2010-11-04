dnl
dnl Permission is hereby granted, free of charge, to any person obtaining a
dnl copy of this software and associated documentation files (the "Software"),
dnl to deal in the Software without restriction, including without limitation
dnl the rights to use, copy, modify, merge, publish, distribute, sublicense,
dnl and/or sell copies of the Software, and to permit persons to whom the
dnl Software is furnished to do so, subject to the following conditions:
dnl
dnl The above copyright notice and this permission notice (including the next
dnl paragraph) shall be included in all copies or substantial portions of the
dnl Software.
dnl
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
dnl THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
dnl LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
dnl FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
dnl DEALINGS IN THE SOFTWARE.
dnl
dnl Author: Paulo Zanoni <pzanoni@mandriva.com>
dnl

AC_PREREQ([2.65])
AC_INIT([xdriverprobe], [0.1], [pzanoni@mandriva.com])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign -Wall -Werror dist-bzip2])
AM_MAINTAINER_MODE
AC_PROG_CC

AC_CHECK_LIB([dl], [dlopen])

PKG_CHECK_MODULES(XDRIVERPROBE, xorg-server)

MODULE_DIR=`pkg-config --variable=moduledir xorg-server`
AC_ARG_WITH(module-dir, AC_HELP_STRING([--with-module-dir],
	    [Change Xorg module dir]), MODULE_DIR="$withval")
AC_DEFINE_UNQUOTED([MODULE_DIR], "$MODULE_DIR", [Xorg driver module dir])

AC_ARG_WITH(extra-module-dir, AC_HELP_STRING([--with-extra-module-dir],
	    [Add an extra Xorg module dir]),
	    AC_DEFINE_UNQUOTED([EXTRA_MODULE_DIR], "$withval",
			       [Extra Xorg module dir]))

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

