#  -*- Mode: makefile; indent-tabs-mode: t -*-
#
#  This file is part of systemd-bootchart
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

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AM_MAKEFLAGS = --no-print-directory
AUTOMAKE_OPTIONS = color-tests parallel-tests

GCC_COLORS ?= 'ooh, shiny!'
export GCC_COLORS

SUBDIRS = .

# remove targets if the command fails
.DELETE_ON_ERROR:

# keep intermediate files
.SECONDARY:

# Keep the test-suite.log
.PRECIOUS: $(TEST_SUITE_LOG) Makefile

# Dirs of external packages
systemdir=@systemdir@

# And these are the special ones for /
rootprefix=@rootprefix@
rootbindir=$(rootprefix)/bin
rootlibexecdir=$(rootprefix)/lib/systemd
pkgsysconfdir=$(sysconfdir)/systemd
systemunitdir=$(rootprefix)/lib/systemd/system

AM_CFLAGS = $(OUR_CFLAGS)
AM_LDFLAGS = $(OUR_LDFLAGS)
AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-DPKGSYSCONFDIR=\"$(pkgsysconfdir)\" \
	-DSYSTEM_CONFIG_UNIT_PATH=\"$(pkgsysconfdir)/system\" \
	-DSYSTEM_DATA_UNIT_PATH=\"$(systemunitdir)\" \
	-DSYSTEMD_BINARY_PATH=\"$(rootlibexecdir)/systemd\" \
	-DROOTPREFIX=\"$(rootprefix)\" \
	-DLIBDIR=\"$(libdir)\" \
	-DROOTLIBDIR=\"$(rootlibdir)\" \
	-DROOTLIBEXECDIR=\"$(rootlibexecdir)\" \
	-I $(top_srcdir)/src

#####################################################

dist_noinst_DATA = \
	LICENSE.LGPL2.1 \
	LICENSE.GPL2

EXTRA_DIST = \
	man/bootchart.conf.xml \
	man/systemd-bootchart.xml \
	man/custom-man.xsl \
	man/standard-conf.xml \
	man/standard-options.xml \
	units/systemd-bootchart.service.in \
	tests/run

MANPAGES = man/bootchart.conf.5 man/systemd-bootchart.1
MANPAGES_ALIAS = man/bootchart.conf.d.5

if COND_man
MAYBE_MANPAGES = $(MANPAGES) $(MANPAGES_ALIAS)
endif
man_MANS = $(MAYBE_MANPAGES)

man/bootchart.conf.d.5: man/bootchart.conf.5

XSLTPROC_FLAGS = \
	--nonet \
	--xinclude \
	--stringparam man.output.quietly 1 \
	--stringparam funcsynopsis.style ansi \
	--stringparam man.authors.section.enabled 0 \
	--stringparam man.copyright.section.enabled 0 \
	--stringparam systemd.version $(VERSION) \
	--path '$(builddir)/man:$(srcdir)/man'

XSLT = $(if $(XSLTPROC), $(XSLTPROC), xsltproc)
XSLTPROC_PROCESS_MAN = \
	$(AM_V_XSLT)$(XSLT) -o $@ $(XSLTPROC_FLAGS) $(srcdir)/man/custom-man.xsl $<

man/%.1: man/%.xml man/custom-man.xsl
	$(XSLTPROC_PROCESS_MAN)

man/%.5: man/%.xml man/custom-man.xsl
	$(XSLTPROC_PROCESS_MAN)

#####################################################

libutils_la_SOURCES = \
	src/alloc-util.c \
	src/alloc-util.h \
	src/architecture.h \
	src/attributes.h \
	src/build.h \
	src/cgroup-util.c \
	src/cgroup-util.h \
	src/conf-files.c \
	src/conf-files.h \
	src/conf-parser.c \
	src/conf-parser.h \
	src/def.h \
	src/dirent-util.c \
	src/dirent-util.h \
	src/fd-util.c \
	src/fd-util.h \
	src/fileio.c \
	src/fileio.h \
	src/formats-util.h \
	src/fs-util.c \
	src/fs-util.h \
	src/hash-funcs.c \
	src/hash-funcs.h \
	src/hashmap.c \
	src/hashmap.h \
	src/io-util.c \
	src/io-util.h \
	src/list.h \
	src/log.c \
	src/log.h \
	src/macro.h \
	src/mempool.c \
	src/mempool.h \
	src/missing.h \
	src/parse-util.c \
	src/parse-util.h \
	src/path-util.c \
	src/path-util.h \
	src/process-util.c \
	src/process-util.h \
	src/random-util.c \
	src/random-util.h \
	src/_sd-common.h \
	src/sd-messages.h \
	src/set.h \
	src/siphash24.c \
	src/siphash24.h \
	src/stdio-util.h \
	src/string-util.c \
	src/string-util.h \
	src/strv.c \
	src/strv.h \
	src/strxcpyx.c \
	src/strxcpyx.h \
	src/terminal-util.c \
	src/terminal-util.h \
	src/time-util.c \
	src/time-util.h \
	src/unaligned.h \
	src/utf8.c \
	src/utf8.h \
	src/util.c \
	src/util.h

libutils_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBSYSTEMD_CFLAGS)

libutils_la_LIBADD = \
	$(LIBSYSTEMD_LIBS)

noinst_LTLIBRARIES = libutils.la

#####################################################

systemd_bootchart_SOURCES = \
	src/bootchart.c \
	src/bootchart.h \
	src/store.c \
	src/store.h \
	src/svg.c \
	src/svg.h

systemd_bootchart_LDADD = \
	libutils.la

#####################################################

TESTS = tests/run

substitutions = \
	'|rootlibexecdir=$(rootlibexecdir)|'

SED_PROCESS = \
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && \
	$(SED) $(subst '|,-e 's|@,$(subst =,\@|,$(subst |',|g',$(substitutions)))) \
		< $< > $@

units/%: units/%.in
	$(SED_PROCESS)

man/%: man/%.in
	$(SED_PROCESS)

rootlibexec_PROGRAMS = systemd-bootchart
dist_pkgsysconf_DATA = src/bootchart.conf
nodist_systemunit_DATA = units/systemd-bootchart.service

in_files = $(filter %.in,$(EXTRA_DIST))

CLEANFILES = \
	$(MANPAGES) $(MANPAGES_ALIAS) \
	$(pkgconfigdata_DATA) \
	$(pkgconfiglib_DATA) \
	$(in_files:.in=)

install-exec-hook: $(INSTALL_EXEC_HOOKS)

uninstall-hook: $(UNINSTALL_DATA_HOOKS) $(UNINSTALL_EXEC_HOOKS)

install-data-hook: $(INSTALL_DATA_HOOKS)

.PHONY: git-tag
git-tag:
	git tag -s "v$(VERSION)" -m "systemd $(VERSION)"

.PHONY: git-tar
git-tar:
	git archive --format=tar --prefix=systemd-$(VERSION)/ HEAD | xz > systemd-$(VERSION).tar.xz
