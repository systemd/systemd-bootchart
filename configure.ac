#
#  This file is part of systemd-bootchart.
#
#  systemd is free software; you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation; either version 2.1 of the License, or
#  (at your option) any later version.
#
#  systemd is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with systemd; If not, see <http://www.gnu.org/licenses/>.

AC_PREREQ([2.64])

AC_INIT([systemd-bootchart],
        [235],
        [https://github.com/systemd/systemd-bootchart/issues],
        [systemd-bootchart],
        [https://github.com/systemd/systemd-bootchart/])

AC_CONFIG_SRCDIR([src/bootchart.c])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])

AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_PREFIX_DEFAULT([/usr])
AM_MAINTAINER_MODE([enable])
AM_INIT_AUTOMAKE([foreign 1.11 -Wall -Wno-portability silent-rules tar-pax no-dist-gzip dist-xz subdir-objects parallel-tests])
AM_SILENT_RULES([yes])
AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED([CANONICAL_HOST], "$host", [Canonical host string.])
LT_PREREQ(2.2)
LT_INIT([disable-static])

AC_PROG_CC_C99

SET_ARCH(X86_64, x86_64*)
SET_ARCH(IA32, i*86*)
SET_ARCH(MIPS, mips*)
SET_ARCH(AARCH64, aarch64*)
SET_ARCH(RISCV64, riscv64*)

AC_CHECK_SIZEOF(pid_t)
AC_CHECK_SIZEOF(uid_t)
AC_CHECK_SIZEOF(gid_t)
AC_CHECK_SIZEOF(time_t)
AC_CHECK_SIZEOF(dev_t)
AC_CHECK_SIZEOF(rlim_t,,[
	#include <sys/time.h>
	#include <sys/resource.h>
])

AC_CHECK_DECLS([gettid, getrandom], [], [], [[
#include <sys/types.h>
#include <unistd.h>
#include <sys/mount.h>
#include <linux/random.h>
]])

AC_ARG_WITH(libsystemd,
        AS_HELP_STRING([--without-libsystemd], [Disable use of libsystemd for journal output]),
        [], [with_libsystemd=yes])

AS_IF([test "x$with_libsystemd" != xno],
        [PKG_CHECK_MODULES(LIBSYSTEMD, [libsystemd >= 221],
                [AC_DEFINE([HAVE_LIBSYSTEMD],[1],[Define if you have libsystemd])],
                [AC_MSG_ERROR([*** libsystemd library not found])]
        )]
)

CC_CHECK_FLAGS_APPEND([with_cflags], [CFLAGS], [\
        -pipe \
        -Wall \
        -Wextra \
        -Wundef \
        "-Wformat=2 -Wformat-security -Wformat-nonliteral" \
        -Wlogical-op \
        -Wmissing-include-dirs \
        -Wold-style-definition \
        -Wpointer-arith \
        -Winit-self \
        -Wdeclaration-after-statement \
        -Wfloat-equal \
        -Wsuggest-attribute=noreturn \
        -Werror=missing-prototypes \
        -Werror=implicit-function-declaration \
        -Werror=missing-declarations \
        -Werror=return-type \
        -Werror=shadow \
        -Wstrict-prototypes \
        -Wredundant-decls \
        -Wmissing-noreturn \
        -Wshadow \
        -Wendif-labels \
        -Wstrict-aliasing=2 \
        -Wwrite-strings \
        -Wno-unused-parameter \
        -Wno-missing-field-initializers \
        -Wno-unused-result \
        -Wno-format-signedness \
        -Werror=overflow \
        -Wdate-time \
        -Wnested-externs \
        -ffast-math \
        -fno-common \
        -fdiagnostics-show-option \
        -fno-strict-aliasing \
        -fvisibility=hidden \
        -fstack-protector \
        -fstack-protector-strong \
        -fPIE \
        --param=ssp-buffer-size=4])

AS_CASE([$CC], [*clang*],
        [CC_CHECK_FLAGS_APPEND([with_cppflags], [CPPFLAGS], [\
               -Wno-typedef-redefinition \
               -Wno-gnu-variable-sized-type-not-at-end \
        ])])

AS_CASE([$CFLAGS], [*-O[[12345sz\ ]]*],
        [CC_CHECK_FLAGS_APPEND([with_cflags], [CFLAGS], [\
               -flto -ffat-lto-objects])],
        [AC_MSG_RESULT([skipping -flto, optimization not enabled])])
AC_SUBST([OUR_CFLAGS], "$with_cflags $sanitizer_cflags")

AS_CASE([$CFLAGS], [*-O[[12345sz\ ]]*],
        [CC_CHECK_FLAGS_APPEND([with_cppflags], [CPPFLAGS], [\
               -Wp,-D_FORTIFY_SOURCE=2])],
        [AC_MSG_RESULT([skipping -D_FORTIFY_SOURCE, optimization not enabled])])
AC_SUBST([OUR_CPPFLAGS], "$with_cppflags $sanitizer_cppflags")

AS_CASE([$CFLAGS], [*-O[[12345sz\ ]]*],
        [CC_CHECK_FLAGS_APPEND([with_ldflags], [LDFLAGS], [\
               -Wl,--gc-sections])],
        [AC_MSG_RESULT([skipping --gc-sections, optimization not enabled])])
AC_SUBST([OUR_CFLAGS], "$with_ldflags $sanitizer_cflags")

AS_CASE([$CFLAGS], [*-O[[12345sz\ ]]*],
        [CC_CHECK_FLAGS_APPEND([with_cflags], [CFLAGS], [\
               -ffunction-sections -fdata-sections])],
        [AC_MSG_RESULT([skipping -ffunction/data-section, optimization not enabled])])
AC_SUBST([OUR_CFLAGS], "$with_cflags $sanitizer_cflags")

CC_CHECK_FLAGS_APPEND([with_ldflags], [LDFLAGS], [\
        -Wl,--as-needed \
        -Wl,--no-undefined \
        -Wl,-z,relro \
        -Wl,-z,now \
        -pie])
AC_SUBST([OUR_LDFLAGS], "$with_ldflags $sanitizer_ldflags")

AC_ARG_WITH([rootprefix],
        AS_HELP_STRING([--with-rootprefix=DIR], [rootfs directory prefix for config files and binaries necessary for boot]),
        [], [with_rootprefix=${ac_default_prefix}])
# --with-rootprefix= (empty) should default to "/" but AX_NORMALIZE_PATH
# defaults those to ".", solve that here for now until we can find a suitable
# fix for AX_NORMALIZE_PATH upstream at autoconf-archive.
# See: https://github.com/systemd/systemd/issues/54
if test "x${with_rootprefix}" = "x"; then
        with_rootprefix="/"
fi
AX_NORMALIZE_PATH([with_rootprefix])

AC_ARG_WITH([rootlibdir],
        AS_HELP_STRING([--with-rootlibdir=DIR], [Root directory for libraries necessary for boot]),
        [],
        [with_rootlibdir=${libdir}])

AC_SUBST([rootprefix], [$with_rootprefix])
AC_SUBST([rootlibdir], [$with_rootlibdir])

AC_ARG_ENABLE([man],
        AS_HELP_STRING([--disable-man],[Build the man pages (default: yes)]),
        [build_man=$enableval],
        [build_man=yes])

AC_PATH_PROG([XSLTPROC], [xsltproc])
AS_IF([test -z "$XSLTPROC" -a "$build_man" = "yes"],
                [AC_MSG_ERROR([*** xsltproc is required for man pages])])

AM_CONDITIONAL([COND_man],[test "$build_man" = "yes"])

AC_CONFIG_FILES([
        Makefile
])

AC_OUTPUT
AC_MSG_RESULT([
        $PACKAGE_NAME $VERSION

        prefix:                  ${prefix}
        sysconf dir:             ${sysconfdir}
        datarootdir:             ${datarootdir}
        includedir:              ${includedir}
        lib dir:                 ${libdir}
        rootlib dir:             ${with_rootlibdir}

        CFLAGS:                  ${OUR_CFLAGS} ${CFLAGS}
        CPPFLAGS:                ${OUR_CPPFLAGS} ${CPPFLAGS}
        LDFLAGS:                 ${OUR_LDFLAGS} ${LDFLAGS}
])
